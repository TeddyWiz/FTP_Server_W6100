/*
* Wiznet.
* (c) Copyright 2002, Wiznet.
*
* Filename  : ftpd.c
* Version   : 1.0
* Programmer(s) :
* Created   : 2003/01/28
* Description   : FTP daemon. (AVR-GCC Compiler)
*/


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
//#include "stdio_private.h"
#include "socket.h"
#include "ftpd.h"
#include "ff_func.h"

/* Command table */
static char *commands[] = {
    "user",
    "acct",
    "pass",
    "type",
    "list",
    "cwd",
    "dele",
    "name",
    "quit",
    "retr",
    "stor",
    "port",
    "eprt",
    "nlst",
    "pwd",
    "xpwd",
    "mkd",
    "xmkd",
    "xrmd",
    "rmd ",
    "stru",
    "mode",
    "syst",
    "xmd5",
    "xcwd",
    "feat",
    "pasv",
    "epsv",
    "size",
    "mlsd",
    "appe",
    NULL
};

#if 0
/* Response messages */
char banner[] = "220 %s FTP version %s ready.\r\n";
char badcmd[] = "500 Unknown command '%s'\r\n";
char binwarn[] = "100 Warning: type is ASCII and %s appears to be binary\r\n";
char unsupp[] = "500 Unsupported command or option\r\n";
char givepass[] = "331 Enter PASS command\r\n";
char logged[] = "230 Logged in\r\n";
char typeok[] = "200 Type %s OK\r\n";
char only8[] = "501 Only logical bytesize 8 supported\r\n";
char deleok[] = "250 File deleted\r\n";
char mkdok[] = "200 MKD ok\r\n";
char delefail[] = "550 Delete failed: %s\r\n";
char pwdmsg[] = "257 \"%s\" is current directory\r\n";
char badtype[] = "501 Unknown type \"%s\"\r\n";
char badport[] = "501 Bad port syntax\r\n";
char unimp[] = "502 Command does not implemented yet.\r\n";
char bye[] = "221 Goodbye!\r\n";
char nodir[] = "553 Can't read directory \"%s\": %s\r\n";
char cantopen[] = "550 Can't read file \"%s\": %s\r\n";
char sending[] = "150 Opening data connection for %s (%d.%d.%d.%d,%d)\r\n";
char cantmake[] = "553 Can't create \"%s\": %s\r\n";
char writerr[] = "552 Write error: %s\r\n";
char portok[] = "200 PORT command successful.\r\n";
char rxok[] = "226 Transfer complete.\r\n";
char txok[] = "226 Transfer complete.\r\n";
char noperm[] = "550 Permission denied\r\n";
char noconn[] = "425 Data connection reset\r\n";
char lowmem[] = "421 System overloaded, try again later\r\n";
char notlog[] = "530 Please log in with USER and PASS\r\n";
char userfirst[] = "503 Login with USER first.\r\n";
char okay[] = "200 Ok\r\n";
char syst[] = "215 %s Type: L%d Version: %s\r\n";
char sizefail[] = "550 File not found\r\n";
#endif

uint8_t remote_ip[16]={0,};
uint16_t  remote_port;
uint8_t local_ip[16];
uint16_t  local_port;

struct ftpd ftp;


int ftpd_dsock_ready();
int ftpd_listcmd(char* arg);
int ftpd_retrcmd(char* arg);
int ftpd_storcmd(char* arg);
int proc_ftpd(char * buf);
char ftplogin(char * pass);
int pport(char * arg);
int pport_extend(char * arg);



void ftpd_init(char* user, char* pass)
{
    uint8_t i;
    uint8_t status;
    ftp.is_login = 0;
    ftp.is_active = 1;
    strcpy(ftp.user, user);
    strcpy(ftp.pass, pass);
    local_port = 35000;
    strcpy(ftp.workingdir, "/");
    for(i = 0; i < SOCK_MAX_NUM; i++)
    {
        getsockopt(i, SO_STATUS, &status);
        if(status == SOCK_CLOSED)
        {
            ftp.control = i++;
            break;
        }
    }
    for(; i < SOCK_MAX_NUM; i++)
    {
        getsockopt(i, SO_STATUS, &status);
        if(status == SOCK_CLOSED)
        {
            ftp.data = i;
            break;;
        }
    }
}

uint8_t ftpd_run(uint8_t * dbuf)
{
    uint8_t tmp;
    uint16_t size = 0, i;
    int ret = 0;

    getsockopt(ftp.control, SO_STATUS, &tmp);
    switch(tmp)
    {

        case SOCK_ESTABLISHED :
            ctlsocket(ftp.control,CS_GET_INTERRUPT,&tmp);
            if(tmp & Sn_IR_CON)
            {
                tmp = Sn_IR_CON;
                ctlsocket(ftp.control,CS_CLR_INTERRUPT,&tmp);
                printf("%d:FTP Connected\r\n", ftp.control);
                //fsprintf(ftp.control, banner, HOSTNAME, VERSION);
                strcpy(ftp.workingdir, "/");
                sprintf((char *)dbuf, "220 %s FTP version %s ready.\r\n", HOSTNAME, VERSION);
                ret = send(ftp.control, (uint8_t *)dbuf, strlen((const char *)dbuf));
                if(ret < 0)
                {
                    printf("%d:send() error:%d\r\n",ftp.control,ret);
                    close(ftp.control);
                    return ret;
                }

                wiz_NetInfo gWIZNETINFO;
                ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);
                getsockopt(ftp.control, SO_EXTSTATUS, &tmp);
                if(tmp&Sn_ESR_TCPM_IPV6)
                {
                    for(i=0; i<16; i++) local_ip[i] = gWIZNETINFO.gua[i];
                    ftp.as_type=AS_IPV6;
                }
                else
                {
                    for(i=0; i<4; i++) local_ip[i] = gWIZNETINFO.ip[i];
                    ftp.as_type=AS_IPV4;
                }

            }
            getsockopt(ftp.control, SO_RECVBUF, &size);
            if((size ) > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
            {
                printf("size: %d\r\n", size);

                memset(dbuf, 0, _FTP_MAX_MSG_SIZE_);

                if(size > _FTP_MAX_MSG_SIZE_) size = _FTP_MAX_MSG_SIZE_ - 1;

                ret = recv(ftp.control,dbuf,size);
                dbuf[ret] = '\0';
                if(ret != size)
                {
                    if(ret==SOCK_BUSY) return 0;
                    if(ret < 0)
                    {
                        printf("%d:recv() error:%d\r\n",ftp.control,ret);
                        close(ftp.control);
                        return ret;
                    }
                }
                printf("Rcvd Command: %s", dbuf);
                proc_ftpd((char *)dbuf);
            }

            break;

        case SOCK_CLOSE_WAIT :
            printf("%d:CloseWait\r\n",ftp.control);
            if((ret=disconnect(ftp.control)) != SOCK_OK) return ret;
            printf("%d:Closed\r\n",ftp.control);
            break;

        case SOCK_CLOSED :
            printf("%d:FTPStart\r\n",ftp.control);
            if((ret=socket(ftp.control, Sn_MR_TCPD, IPPORT_FTP, 0x0)) != ftp.control)
            {
                printf("%d:socket() error:%d\r\n", ftp.control, ret);
                close(ftp.control);
                return ret;
            }
            break;

        case SOCK_INIT :
            printf("%d:Opened\r\n",ftp.control);
            //strcpy(ftp.workingdir, "/");
            if( (ret = listen(ftp.control)) != SOCK_OK)
            {
                printf("%d:Listen error\r\n",ftp.control);
                return ret;
            }
            printf("%d:Listen ok\r\n",ftp.control);
            break;

        case  SOCK_LISTEN :
            break;

        default :
            #ifdef _FTP_DEBUG_
            printf("FTP Control socket Status:%02X [%s][%d]\r\n", tmp, __FILE__, __LINE__);
            #endif
            break;
    }

    return 0;
}

int proc_ftpd(char * buf)
{
    char *cp, *arg, **cmdp;
    int slen;
    char sendbuf[200];
    char* tmpstr;

    /* Translate first word to lower case */
    for (cp = buf; *cp != ' ' && *cp != '\0'; cp++)
        *cp = tolower(*cp);

    /* Find command in table; if not present, return syntax error */
    for (cmdp = commands; *cmdp != NULL; cmdp++)
        if (strncmp(*cmdp, buf, strlen(*cmdp)) == 0)
            break;

    if (*cmdp == NULL)
    {
        //fsprintf(ftp.control, badcmd, buf);
        slen = sprintf(sendbuf, "500 Unknown command '%s'\r\n", buf);
        send(ftp.control, (uint8_t *)sendbuf, slen);
        return 0;
    }
    /* Allow only USER, PASS and QUIT before logging in */
    if (ftp.is_login == 0)
    {
        switch(cmdp - commands)
        {
            case USER_CMD:
            case PASS_CMD:
            case QUIT_CMD:
                break;
            default:
                //fsprintf(ftp.control, notlog);
                slen = sprintf(sendbuf, "530 Please log in with USER and PASS\r\n");
                send(ftp.control, (uint8_t *)sendbuf, slen);
                return 0;
        }
    }

    arg = &buf[strlen(*cmdp)];
    while(*arg == ' ') arg++;
    slen = strlen(arg);
    arg[slen - 1] = 0x00;
    arg[slen - 2] = 0x00;

    /* Execute specific command */
    switch (cmdp - commands)
    {
        case USER_CMD :
            printf("USER_CMD : %s\r\n", arg);
            if(strcmp(arg,ftp.user)==0)
            {
                slen = sprintf(sendbuf, "331 Enter PASS command\r\n");
                send(ftp.control, (uint8_t *)sendbuf, slen);
            }
            else
            {
                slen = sprintf(sendbuf, "530 Not logged in\r\n");
                send(ftp.control, (uint8_t *)sendbuf, slen);
            }
            break;

        case PASS_CMD :
            printf("PASS_CMD : %s\r\n", arg);
            if(strcmp(arg,ftp.pass)==0)
            {
               printf("%s logged in\r\n", ftp.username);
               slen = sprintf(sendbuf, "230 Logged on\r\n");
               send(ftp.control, (uint8_t *)sendbuf, slen);
               ftp.is_login = 1;
            }
            else
            {
               printf("%s Not logged in\r\n", ftp.username);
               slen = sprintf(sendbuf, "530 Not logged in\r\n");
               send(ftp.control, (uint8_t *)sendbuf, slen);
            }
            break;

        case TYPE_CMD :
            switch(arg[0])
            {
                case 'A':
                case 'a':   /* Ascii */
                    ftp.is_ascii = 1;
                    slen = sprintf(sendbuf, (char*)"200 Type set to %s\r\n", arg);
                    send(ftp.control, (uint8_t *)sendbuf, slen);
                    break;

                case 'B':
                case 'b':   /* Binary */
                case 'I':
                case 'i':   /* Image */
                    ftp.is_ascii = 0;
                    slen = sprintf(sendbuf, "200 Type set to %s\r\n", arg);
                    send(ftp.control, (uint8_t *)sendbuf, slen);
                    break;

                default:    /* Invalid */
                    //fsprintf(ftp.control, badtype, arg);
                    slen = sprintf(sendbuf, "501 Unknown type \"%s\"\r\n", arg);
                    send(ftp.control, (uint8_t *)sendbuf, slen);
                    break;
            }
            break;

        case FEAT_CMD :
            slen = sprintf(sendbuf, "211-Features:\r\n MDTM\r\n REST STREAM\r\n SIZE\r\n MLST size*;type*;create*;modify*;\r\n MLSD\r\n UTF8\r\n CLNT\r\n MFMT\r\n211 END\r\n");
            send(ftp.control, (uint8_t *)sendbuf, slen);
            break;

        case QUIT_CMD :
            printf("QUIT_CMD\r\n");
            slen = sprintf(sendbuf, "221 Goodbye!\r\n");
            send(ftp.control, (uint8_t *)sendbuf, slen);
            disconnect(ftp.control);
            break;

        case RETR_CMD :
            printf("RETR_CMD\r\n");
            ftpd_retrcmd(arg);
            break;

        case APPE_CMD :
        case STOR_CMD:
            printf("STOR_CMD\r\n");
            ftpd_storcmd(arg);
            printf("STOR_CMD FINISH \r\n");
            break;

        case PORT_CMD:
            printf("PORT_CMD\r\n");
            if (pport(arg) == -1){
                slen = sprintf(sendbuf, "501 Bad port syntax\r\n");
                send(ftp.control, (uint8_t *)sendbuf, slen);
            } else{
                ftp.is_active = 1;
                slen = sprintf(sendbuf, "200 PORT command successful.\r\n");
                send(ftp.control, (uint8_t *)sendbuf, slen);
            }
            ftpd_dsock_ready();

            break;

        case EPRT_CMD:
            printf("EPRT_CMD\r\n");
            if (pport_extend(arg) == -1){
                slen = sprintf(sendbuf, "501 Bad port syntax\r\n");
                send(ftp.control, (uint8_t *)sendbuf, slen);
            } else{
                ftp.is_active = 1;
                slen = sprintf(sendbuf, "200 EPRT command successful.\r\n");
                send(ftp.control, (uint8_t *)sendbuf, slen);
            }
            ftpd_dsock_ready();
            break;

        case MLSD_CMD:
            printf("MLSD_CMD\r\n");
            ftpd_listcmd(arg);

            break;

        case LIST_CMD:
            printf("LIST_CMD\r\n");
            ftpd_listcmd(arg);
            break;

        case NLST_CMD:
            printf("NLST_CMD\r\n");
            break;

        case SYST_CMD:
            slen = sprintf(sendbuf, "215 UNIX emulated by WIZnet\r\n");
            send(ftp.control, (uint8_t *)sendbuf, slen);
            break;

        case PWD_CMD:
        case XPWD_CMD:
            slen = sprintf(sendbuf, "257 \"%s\" is current directory.\r\n", ftp.workingdir);
            send(ftp.control, (uint8_t *)sendbuf, slen);
            break;

        case PASV_CMD:
            slen = sprintf(sendbuf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", local_ip[0], local_ip[1], local_ip[2], local_ip[3], local_port >> 8, local_port & 0x00ff);
            send(ftp.control, (uint8_t *)sendbuf, slen);
            ftp.is_active = 0;
            ftpd_dsock_ready();
            printf("PASV port: %d\r\n", local_port);

        break;

        case EPSV_CMD:
            slen = sprintf(sendbuf, "229 Entering Extended Passive Mode (|||%d|)\r\n", local_port );
            send(ftp.control, (uint8_t *)sendbuf, slen);
            ftp.is_active = 0;
            ftpd_dsock_ready();
            printf("EPSV port: %d\r\n", local_port);
        break;

        case SIZE_CMD:
            if(slen > 3)
            {
#if defined(F_FILESYSTEM)
                tmpstr = strrchr(arg, '/');
                *tmpstr = 0;
				slen = get_filesize(arg, tmpstr + 1);
#else
                slen = _FTP_MAX_MSG_SIZE_;
#endif
                slen = sprintf(sendbuf, "213 %d\r\n", slen);
            }
            else
            {
                slen = sprintf(sendbuf, "550 File not Found\r\n");
            }
            send(ftp.control, (uint8_t *)sendbuf, slen);
            break;

        case CWD_CMD:
            if(slen > 3)
            {
                arg[slen - 3] = 0x00;
                tmpstr = strrchr(arg, '/');
                *tmpstr = 0;
#if defined(F_FILESYSTEM)
				slen = get_filesize(arg, tmpstr + 1);
#else
				slen = 0;
#endif
                *tmpstr = '/';
#if defined(_FTP_DEBUG_)
                printf("tmpstr: %s\r\n", tmpstr);
#endif

                if(slen == 0){
                    slen = sprintf(sendbuf, "213 %d\r\n", slen);
                    
#if defined(_FTP_DEBUG_)
                    printf("CWD_CMD 213[%d] : %s\r\n", slen, sendbuf);
#endif
                    strcpy(ftp.workingdir, arg);
                    slen = sprintf(sendbuf, "250 CWD successful. \"%s\" is current directory.\r\n", ftp.workingdir);
                }
				else
				{
					slen = sprintf(sendbuf, "550 CWD failed. \"%s\"\r\n", arg);
				}
            }
            else
            {
                strcpy(ftp.workingdir, arg);
                slen = sprintf(sendbuf, "250 CWD successful. \"%s\" is current directory.\r\n", ftp.workingdir);
            }
#if defined(_FTP_DEBUG_)
            printf("CWD_CMD[%d] : %s\r\n", slen, sendbuf);
#endif
            send(ftp.control, (uint8_t *)sendbuf, slen);
            break;

        case MKD_CMD:
        case XMKD_CMD:
#if defined(F_FILESYSTEM)
            if (f_mkdir(arg) != 0)
            {
                slen = sprintf(sendbuf, "550 Can't create directory. \"%s\"\r\n", arg);
            }
            else
            {
            	slen = sprintf(sendbuf, "257 MKD command successful. \"%s\"\r\n", arg);
            	//strcpy(ftp.workingdir, arg);
            }
#else
            slen = sprintf(sendbuf, "550 Can't create directory. Permission denied\r\n");
#endif
#if defined(_FTP_DEBUG_)
            printf("XMKD_CMD[%d] : %s\r\n", slen, sendbuf);
#endif

            send(ftp.control, (uint8_t *)sendbuf, slen);
            break;

        case DELE_CMD:
#if defined(F_FILESYSTEM)
            if (f_unlink(arg) != 0)
            {
                slen = sprintf(sendbuf, "550 Could not delete. \"%s\"\r\n", arg);
            }
            else
            {
                slen = sprintf(sendbuf, "250 Deleted. \"%s\"\r\n", arg);
            }
#else
            slen = sprintf(sendbuf, "550 Could not delete. Permission denied\r\n");
#endif
#if defined(_FTP_DEBUG_)
            printf("DELE_CMD[%d] : %s\r\n", slen, sendbuf);
#endif
            send(ftp.control, (uint8_t *)sendbuf, slen);
            break;

        case XCWD_CMD:
        case ACCT_CMD:
        case XRMD_CMD:
        case RMD_CMD:
        case STRU_CMD:
        case MODE_CMD:
        case XMD5_CMD:
            //fsprintf(ftp.control, unimp);
            slen = sprintf(sendbuf, "502 Command does not implemented yet.\r\n");
            send(ftp.control, (uint8_t *)sendbuf, slen);
            break;

        default:    /* Invalid */
            //fsprintf(ftp.control, badcmd, arg);
            slen = sprintf(sendbuf, "500 Unknown command \'%s\'\r\n", arg);
            send(ftp.control, (uint8_t *)sendbuf, slen);
            break;
    }

    return 1;
}

#if defined(F_FILESYSTEM)
void print_filedsc(FIL *fil)
{
#if defined(_FTP_DEBUG_)
	printf("File System pointer : %08lX\r\n", fil->obj.fs);
	printf("File System mount ID : %d\r\n", fil->obj.id);
	printf("File status flag : %08lX\r\n", fil->flag);
	printf("File System pads : %08lX\r\n", fil->err);
	printf("File read write pointer : %08lX\r\n", fil->fptr);
	printf("File size : %08lX\r\n", fil->obj.objsize);
	printf("File start cluster : %08lX\r\n", fil->obj.sclust);
	printf("current cluster : %08lX\r\n", fil->clust);
	printf("current data sector : %08lX\r\n", fil->sect);
	printf("dir entry sector : %08lX\r\n", fil->dir_sect);
	printf("dir entry pointer : %08lX\r\n", fil->dir_ptr);
#endif
}
#endif




int pport(char * arg)
{
    int i;
    char* tok=0;

    for (i = 0; i < 4; i++)
    {
        if(i==0)tok = strtok(arg,",");
        else tok = strtok(NULL,",");
        //remote_ip[i] = (uint8_t)atoi(tok, 10);
        remote_ip[i] = (uint8_t)atoi(tok);
        if (!tok)
        {
            printf("bad pport : %s\r\n", arg);
            return -1;
        }
    }
    remote_port = 0;
    for (i = 0; i < 2; i++)
    {
        tok = strtok(NULL,",");
        remote_port <<= 8;
        //remote_port += atoi(tok, 10);
        remote_port += atoi(tok);
        if (!tok)
        {
            printf("bad pport : %s\r\n", arg);
            return -1;
        }
    }
    printf("ip : %d.%d.%d.%d, port : %d\r\n", remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3], remote_port);

    return 0;
}

int pport_extend(char * arg)
{
    char* tok=0;
    char tok2hex[100]={0.};
    uint8_t tok_ver, zero_padding=7, addr_cnt=0, addr_index=0, i,j;
    int size;
    tok = strtok(arg,"|");

    //tok_ver=(uint8_t)atoi(tok,10);
    tok_ver=(uint8_t)atoi(tok);

    tok=strtok(NULL,"|");
    size = strlen((char*)tok);
    if(tok_ver==2){
        for(i=0; i<size; i++){
            if(tok[i]==':') zero_padding--;
            else if((tok[i]>='0') && (tok[i]<='9')) tok2hex[i]=tok[i]-'0';
            else if((tok[i]>='a') && (tok[i]<='f')) tok2hex[i]=tok[i]-'a'+10;
            else if((tok[i]>='A') && (tok[i]<='F')) tok2hex[i]=tok[i]-'A'+10;
        }

        if(tok[0]==':' || tok[size-1]==':') zero_padding++;

        for(i=0; i<=size; i++){
            if(tok[i]==':'||i==size){
                if(addr_cnt==0){
                    for(j=0; j<=zero_padding; j++){
                        remote_ip[addr_index++]=0;
                        remote_ip[addr_index++]=0;
                    }
                }
                else if(addr_cnt==1){
                    remote_ip[addr_index++]=0;
                    remote_ip[addr_index++]=tok2hex[i-1];
                }
               else if(addr_cnt==2){
                   remote_ip[addr_index++]=0;
                   remote_ip[addr_index++]= (uint8_t)tok2hex[i-2]<<4 |tok2hex[i-1];
               }
               else if(addr_cnt==3){
                   remote_ip[addr_index++]=tok2hex[i-3];
                   remote_ip[addr_index++]=(uint8_t)tok2hex[i-2]<<4 | tok2hex[i-1];
               }
               else if(addr_cnt==4){
                   remote_ip[addr_index++]=(uint8_t)tok2hex[i-4]<<4 | tok2hex[i-3];
                   remote_ip[addr_index++]=(uint8_t)tok2hex[i-2]<<4 | tok2hex[i-1];
               }
                addr_cnt=0;
            }
            else
                addr_cnt++;
        }
    }
    else if(tok_ver==1){
        for(i=0; i<size; i++){
            if((tok[i]>='0') && (tok[i]<='9')) tok2hex[i]=tok[i]-'0';
        }

        for(i=0; i<size; i++){
            if(tok[i]=='.' || tok[i]=='\0'){
                if(addr_cnt==1)
                    remote_ip[addr_index++]=tok2hex[i-1];
               else if(addr_cnt==2)
                   remote_ip[addr_index++]= (uint8_t)tok2hex[i-2]*10 + tok2hex[i-1];
               else if(addr_cnt==3)
                   remote_ip[addr_index++]=(uint8_t)tok2hex[i-3]*100 + (uint8_t)tok2hex[i-2]*10 + tok2hex[i-1];
                addr_cnt=0;
            }
            else
                addr_cnt++;
        }
    }
    else
    {
    printf("bad pport : %s\r\n", arg);
        return -1;
    }

    tok=strtok(NULL,"|");
    if (!tok)
    {
        printf("bad pport : %s\r\n", arg);
        return -1;
    }
    remote_port=(uint16_t)atoi(tok);
    return 0;
}



int ftpd_dsock_ready(){
    uint8_t tmp;
    int ret;
    getsockopt(ftp.data, SO_STATUS, &tmp);
    if(tmp != SOCK_CLOSED)
    {
        close(ftp.data);
        #ifdef _FTP_DEBUG_
        printf("%d:FTP data Init socket [%s][%d]\r\n", ftp.data, __FILE__, __LINE__);
        #endif
    }
    if(ftp.is_active == 1){
        socket(ftp.data, Sn_MR_TCPD, IPPORT_FTPD, 0x0);
        if(ftp.as_type==AS_IPV6)
            ret=connect(ftp.data, remote_ip, remote_port, 16);
        else
            ret=connect(ftp.data, remote_ip, remote_port, 4);
        if(ret  != SOCK_OK){
            printf("%d:Connect error\r\n", ftp.data);
            return ret;
        }

    }
    else
    {
        if((ret=socket(ftp.data, Sn_MR_TCPD, local_port, 0x0)) != ftp.data)
        {
            printf("%d:socket() error:%d\r\n", ftp.data, ret);
            close(ftp.data);
            return ret;
        }
        if( (ret = listen(ftp.data)) != SOCK_OK)
        {
            printf("%d:Listen error\r\n",ftp.data);
            return ret;
        }
        #ifdef _FTP_DEBUG_
        printf("%d:FTP data socket open & Listen [%s][%d]\r\n", ftp.data, __FILE__, __LINE__);
        #endif
    }
    return 0;
}

int ftpd_listcmd(char* arg){
    char sendbuf[200], dbuf[_FTP_MAX_MSG_SIZE_];
    uint8_t tmp;
    int size;
    int16_t slen;
    FRESULT ret;

    #ifdef _FTP_DEBUG_
    printf("%s->%s [%d]\r\n",  __FILE__, __FUNCTION__, __LINE__);
    #endif
    slen = sprintf(sendbuf, "150 Opening data channel for directory listing of \"%s\"\r\n", ftp.workingdir);
    send(ftp.control, (uint8_t *)sendbuf, slen);
    getsockopt(ftp.data, SO_STATUS, &tmp);
    if(tmp!=SOCK_ESTABLISHED)
    {
        ftpd_dsock_ready();
    }

#if defined(F_FILESYSTEM)
    ret = scan_files(ftp.workingdir, dbuf, (int *)&size);
    if(ret != FR_OK)
    {
        size = 0;
        sprintf(dbuf,"ERROR\r\n");
    }
#else
    if (strncmp(ftp.workingdir, "/$Recycle.Bin", sizeof("/$Recycle.Bin")) != 0)
        size = sprintf((char*)dbuf, "drwxr-xr-x 1 ftp ftp 0 Dec 31 2014 $Recycle.Bin\r\n-rwxr-xr-x 1 ftp ftp 512 Dec 31 2014 test.txt\r\n");
    size = strlen((char*)dbuf);
#endif
#if defined(_FTP_DEBUG_)
    printf("returned data[%d] : %s[end]\r\n", size, dbuf);
#endif

    if(size > 0)
    {
        send(ftp.data, (uint8_t*)dbuf, size);
    }
    disconnect(ftp.data);
    #ifdef _FTP_DEBUG_
    printf("%d:FTP data disconnect [%s][%d]\r\n", ftp.data, __FILE__, __LINE__);
    #endif
    if(size > 0)
    {
        size = sprintf((char*)dbuf, "226 Successfully transferred \"%s\"\r\n", ftp.workingdir);
    }
    else{
        size = sprintf((char*)dbuf, "451  local error in processing \"%s\"\r\n", ftp.workingdir);
    }
    send(ftp.control, (uint8_t*)dbuf, size);
    
    return 0;
}

int ftpd_retrcmd(char* arg){
    datasize_t slen;
    int32_t remain_filesize;
    uint8_t tmp;
    datasize_t send_byte;
    datasize_t blocklen;
    char sendbuf[500], dbuf[_FTP_MAX_MSG_SIZE_];

#ifdef _FTP_DEBUG_
    printf("%s->%s [%d]\r\n",  __FILE__, __FUNCTION__, __LINE__);
#endif
    if(strlen(ftp.workingdir) == 1)
        sprintf(ftp.filename, "/%s", arg);
    else
        sprintf(ftp.filename, "%s/%s", ftp.workingdir, arg);
    slen = sprintf(sendbuf, "150 Opening data channel for file download from server of \"%s\"\r\n", ftp.filename);
    send(ftp.control, (uint8_t *)sendbuf, slen);
    getsockopt(ftp.data, SO_STATUS, &tmp);
    if(tmp!=SOCK_ESTABLISHED)
    {
        ftpd_dsock_ready();
    }
    printf("%d:filename to retrieve : %s %d\r\n", ftp.data, ftp.filename, strlen(ftp.filename));

#if defined(F_FILESYSTEM)
    ftp.fr = f_open(&(ftp.fil), (const char *)ftp.filename, FA_READ);
    //print_filedsc(&(ftp.fil));
    if(ftp.fr == FR_OK){
        remain_filesize = (int32_t)ftp.fil.obj.objsize;
#if defined(_FTP_DEBUG_)
        printf("f_open return FR_OK\r\n");
#endif
        do{
#if defined(_FTP_DEBUG_)
            printf("remained file size: %d , %ld\r\n",remain_filesize,  ftp.fil.obj.objsize);
#endif
            //memset(dbuf, 0, _FTP_MAX_MSG_SIZE_);

            if(remain_filesize > _FTP_MAX_MSG_SIZE_)
                send_byte = (datasize_t)_FTP_MAX_MSG_SIZE_;
            else
                send_byte = remain_filesize;
#if defined(_FTP_DEBUG_)
            printf("send byte:%d\r\n",send_byte);
#endif
            ftp.fr = f_read(&(ftp.fil), dbuf, send_byte , &blocklen);
            if(ftp.fr != FR_OK)
            {
#if defined(_FTP_DEBUG_)
                printf("file read Error:%d\r\n",ftp.fr);
#endif
                break;
            }
#if defined(_FTP_DEBUG_)
            printf("----->fsize:%d recv:%d len:%d \r\n", remain_filesize, send_byte, blocklen);
            printf("----->fn:%s data:%s \r\n", ftp.filename, dbuf);
#endif
            dbuf[blocklen] = 0;
            send(ftp.data, dbuf, blocklen);
            remain_filesize -= blocklen;
        }while(remain_filesize != 0);
#if defined(_FTP_DEBUG_)
        printf("\r\nFile read finished\r\n");
#endif
        ftp.fr = f_close(&(ftp.fil));
    }
    else
    {
#if defined(_FTP_DEBUG_)
        printf("File Open Error: %d\r\n", ftp.fr);
#endif
    }
#else  //not filesystem
    remain_filesize = strlen(ftp.filename);
    do{
        //memset(dbuf, 0, _FTP_MAX_MSG_SIZE_);
        blocklen = sprintf(dbuf, "%s", ftp.filename);
        printf("########## dbuf:%s\r\n", dbuf);
        send(ftp.data, (uint8_t*)dbuf, blocklen);
        remain_filesize -= blocklen;
    }while(remain_filesize != 0);
#endif
    disconnect(ftp.data);
    slen = sprintf(sendbuf, "226 Successfully transferred \"%s\"\r\n", ftp.filename);
    send(ftp.control, (uint8_t*)sendbuf, slen);
    #if defined(_FTP_DEBUG_)
        printf("send[%d] %d : %s\r\n", ftp.control, slen, sendbuf);
    #endif
    return 0;

}

int ftpd_storcmd(char* arg){
    char sendbuf[500], dbuf[_FTP_MAX_MSG_SIZE_];
    datasize_t slen;
    int16_t remain_datasize;
    uint8_t tmp;
    uint32_t blocklen;
    datasize_t recv_byte;
    datasize_t ret;

#ifdef _FTP_DEBUG_
    printf("%s->%s [%d]\r\n",  __FILE__, __FUNCTION__, __LINE__);
    #endif

    if(strlen(ftp.workingdir) == 1)
       sprintf(ftp.filename, "/%s", arg);
    else
       sprintf(ftp.filename, "%s/%s", ftp.workingdir, arg);
    slen = sprintf(sendbuf, "150 Opening data channel for file upload to server of \"%s\"\r\n", ftp.filename);
    send(ftp.control, (uint8_t *)sendbuf, slen);

    printf("%d:filename to store : %s %d\r\n", ftp.data, ftp.filename, strlen(ftp.filename));
    
#if defined(F_FILESYSTEM)
//#if 0

	ftp.fr = f_open(&(ftp.fil), (const char *)ftp.filename, FA_CREATE_ALWAYS | FA_WRITE);
	//print_filedsc(&(ftp.fil));
	if(ftp.fr == FR_OK){
#if defined(_FTP_DEBUG_)
		printf("f_open return FR_OK\r\n");
#endif
		while(1){
            getsockopt(ftp.data, SO_RECVBUF, &remain_datasize);
			//if((remain_datasize = getSn_RX_RSR(ftp.data)) > 0){
#if defined(_FTP_DEBUG_)
           printf("1_Remain :%d \r\n", remain_datasize);
#endif
			if(remain_datasize > 0){
				while(1){
                    if(remain_datasize > _FTP_MAX_MSG_SIZE_)
                       recv_byte = _FTP_MAX_MSG_SIZE_;
                   else
                       recv_byte = remain_datasize;
#if defined(_FTP_DEBUG_)
                       printf("recv_byte:%d \r\n", recv_byte);
#endif
                   ret = recv(ftp.data, (uint8_t*)dbuf, recv_byte);
                   dbuf[ret]=0;
#if defined(_FTP_DEBUG_)
					printf("----->fn:%s data:%s \r\n", ftp.filename, dbuf);
#endif
                    if(ret < 0)
                    {
#if defined(_FTP_DEBUG_)
                        printf("recv error ret : %d remain : %d \r\n", ret, remain_datasize);
#endif
                        break;
                    }
					ftp.fr = f_write(&(ftp.fil), dbuf, (UINT)ret, &blocklen);
#if defined(_FTP_DEBUG_)
					printf("----->dsize:%d recv:%d len:%d \r\n", remain_datasize, ret, blocklen);
                    printf("ftp.fr = %d\r\n", ftp.fr);
#endif
					remain_datasize -= blocklen;

					if(ftp.fr != FR_OK){
#if defined(_FTP_DEBUG_)
						printf("f_write failed\r\n");
#endif
						break;
					}

					if(remain_datasize <= 0)
						break;
				}

				if(ftp.fr != FR_OK){
#if defined(_FTP_DEBUG_)
					printf("f_write failed\r\n");
#endif
					break;
				}

#if defined(_FTP_DEBUG_)
				printf("#");
#endif
			}else{
                getsockopt(ftp.data, SO_STATUS, &tmp);
               if(tmp != SOCK_ESTABLISHED)
                   break;
			}
		}
#if defined(_FTP_DEBUG_)
		printf("\r\nFile write finished\r\n");
#endif
		ftp.fr = f_close(&(ftp.fil));
#if defined(_FTP_DEBUG_)
        printf("\r\n f_close %d\r\n", ftp.fr);
#endif

	}else{
#if defined(_FTP_DEBUG_)
		printf("File Open Error: %d\r\n", ftp.fr);
#endif
	}

	//fno.fdate = (WORD)(((current_year - 1980) << 9) | (current_month << 5) | current_day);
	//fno.ftime = (WORD)((current_hour << 11) | (current_min << 5) | (current_sec >> 1));
	//f_utime((const char *)ftp.filename, &fno);
#else
    while(1){
        getsockopt(ftp.data, SO_RECVBUF, &remain_datasize);
       if((remain_datasize) > 0){
           while(1){
               if(remain_datasize > _FTP_MAX_MSG_SIZE_)
                   recv_byte = _FTP_MAX_MSG_SIZE_;
               else
                   recv_byte = remain_datasize;
               ret = recv(ftp.data, (uint8_t*)dbuf, recv_byte);
               //dbuf[_FTP_MAX_MSG_SIZE_]=0;
               dbuf[ret]=0;
               printf("########## dbuf[%d][%d]:%s\r\n",recv_byte, ret, dbuf);
#if 1
               if(ret < 0)
               {
            	   printf("recv end %d\r\n", ret);
            	   break;
               }
#endif
               remain_datasize -= ret;
               if(remain_datasize <= 0)
                   break;
           }
       }else{
           getsockopt(ftp.data, SO_STATUS, &tmp);
           if(tmp != SOCK_ESTABLISHED)
               break;
       }
    }
#endif
    disconnect(ftp.data);

    slen = sprintf(sendbuf, "226 Successfully transferred \"%s\"\r\n", ftp.filename);
    printf("send[%d] ctrl[%d]:%s\r\n", ftp.control, slen, sendbuf);
    ret = send(ftp.control, (uint8_t*)sendbuf, slen);
    printf("send ctrl[%d]\r\n",ret);
    return 0;
}
