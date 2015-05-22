#pragma comment(lib,"ws2_32")
#define UNICODE
#include<windows.h>
#define MAX_ATTACHMENT_SIZE (1024*1024*25)

char *pServer="mail.example.com";
char *pFrom="anonymous";
char *pTo;
char *pSubject;
char *pBody;

char convtobase(char c)
{
	if (c <= 0x19)
		return c + 'A';
	if (c >= 0x1a && c <= 0x33)
		return c - 0x1a + 'a';
	if (c >= 0x34 && c <= 0x3d)
		return c - 0x34 + '0';
	if (c == 0x3e)
		return '+';
	if (c == 0x3f)
		return '/';
	return '=';
}

void encode(char*lpszOrg, char*lpszDest)
{ 
	int i = 0, iR = 16;
	lstrcpyA(lpszDest, "=?ISO-2022-JP?B?");
	while (1)
	{ 
		if (lpszOrg[i] == '\0')
		{ 
			break; 
		} 
		lpszDest[iR] = convtobase((lpszOrg[i]) >> 2);
		if (lpszOrg[i + 1] == '\0')
		{
			lpszDest[iR + 1] = convtobase(((lpszOrg[i] & 0x3) << 4) );
			lpszDest[iR + 2] = '=';
			lpszDest[iR + 3] = '=';
			lpszDest[iR + 4] = '\0';
			break;
		}
		lpszDest[iR + 1] = convtobase(((lpszOrg[i] & 0x3) << 4) + ((lpszOrg[i + 1]) >> 4));
		
		if (lpszOrg[i + 2] == '\0')
		{
			lpszDest[iR + 2] = convtobase((lpszOrg[i + 1] & 0xf) << 2);
			lpszDest[iR + 3] = '=';
			lpszDest[iR + 4] = '\0';
			break;
		}
		lpszDest[iR + 2] = convtobase(((lpszOrg[i + 1] & 0xf) << 2) + ((lpszOrg[i + 2]) >> 6)); 
		lpszDest[iR + 3] = convtobase(lpszOrg[i + 2] & 0x3f);
		lpszDest[iR + 4] = '\0';
		i += 3; iR += 4;
	}
	lstrcatA(lpszDest, "?=");
	return;
}

char*sjis2jis(const char*lpszOrg)
{
	char*MultiString=0;
	DWORD dwTextLen=MultiByteToWideChar(932,0,lpszOrg,-1,0,0);
	if(dwTextLen)
	{
		LPWSTR WideString=(LPWSTR)GlobalAlloc(GMEM_FIXED,sizeof(WCHAR)*(dwTextLen+1));
		MultiByteToWideChar(932,0,lpszOrg,-1,WideString,dwTextLen);
		dwTextLen=WideCharToMultiByte(50220,0,WideString,-1,0,0,0,0);
		if(dwTextLen)
		{
			MultiString=(LPSTR)GlobalAlloc(GMEM_FIXED,sizeof(CHAR)*(dwTextLen+1));
			WideCharToMultiByte(50220,0,WideString,-1,MultiString,dwTextLen,0,0);
		}
		GlobalFree(WideString);
	}
	return MultiString;
}

int sendmail(const char*szServerName,const char*lpszFrom,const char*lpszTo,const char*lpszSubject,const char*lpszBody)
{
	char szStr[1024], szStrRcv[1024 * 32];
	WSADATA wsaData;
	LPHOSTENT lpHost;
	LPSERVENT lpServ;
	SOCKET s;
	int iProtocolPort;
	SOCKADDR_IN sockadd;
	char*token;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
	{
		return -1;
	}
	lpHost = gethostbyname(szServerName);
	if (lpHost == NULL)
	{
		return -2;
	} 
	s = socket(PF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		return -3;
	}
	lpServ = getservbyname("mail", NULL);
	if (lpServ == NULL)
	{
		iProtocolPort = htons(IPPORT_SMTP);
	}
	else
	{
		iProtocolPort = lpServ->s_port;
	}
	sockadd.sin_family = AF_INET;
	sockadd.sin_port = iProtocolPort;
	sockadd.sin_addr = *((LPIN_ADDR)*lpHost->h_addr_list);
	if (connect(s, (PSOCKADDR)&sockadd, sizeof(sockadd)))
	{
		return -4;
	}
	recv(s, szStrRcv, sizeof(szStrRcv), 0);
	
	lstrcpyA(szStr, "HELO ");
	lstrcatA(szStr, szServerName);
	lstrcatA(szStr, "\r\n");
	send(s, szStr, lstrlenA(szStr), 0);
	recv(s, szStrRcv, sizeof(szStrRcv), 0);
	
	lstrcpyA(szStr, "MAIL FROM:<");
	lstrcatA(szStr, lpszFrom);
	lstrcatA(szStr, ">\r\n");
	send(s, szStr, lstrlenA(szStr), 0);
	recv(s, szStrRcv, sizeof(szStrRcv), 0);
	
	char *lpszToTemp;
	lpszToTemp=(char*)GlobalAlloc(GMEM_FIXED,lstrlenA(lpszTo)+1);
	lstrcpyA(lpszToTemp,lpszTo);

	char *ctx;
	
	token = strtok_s(lpszToTemp, ";", &ctx);
	while (token != NULL)
	{
		lstrcpyA(szStr, "RCPT TO:<");
		lstrcatA(szStr, token);
		lstrcatA(szStr, ">\r\n");
		send(s, szStr, lstrlenA(szStr), 0);
		recv(s, szStrRcv, sizeof(szStrRcv), 0);
		token = strtok_s(NULL, ";", &ctx);
	}
	
	GlobalFree(lpszToTemp);
	
	lstrcpyA(szStr, "DATA\r\n");
	send(s, szStr, lstrlenA(szStr), 0);
	recv(s, szStrRcv, sizeof(szStrRcv), 0);
	
	// ����
	lpszToTemp=(char*)GlobalAlloc(GMEM_FIXED,lstrlenA(lpszTo)+1);
	lstrcpyA(lpszToTemp,lpszTo);
	token = strtok_s(lpszToTemp, ";", &ctx);
	lstrcpyA(szStr,"To: ");
	if( token != NULL)
	{
		lstrcatA(szStr,token);
		token = strtok_s(NULL, ";", &ctx);
	}
	while (token != NULL)
	{
		lstrcatA(szStr,", ");
		lstrcatA(szStr,token);
		token = strtok_s(NULL, ";", &ctx);
	}
	GlobalFree(lpszToTemp);
	lstrcatA(szStr,"\r\n");
	send(s, szStr, lstrlenA(szStr), 0);
	
	// ����
	if(lpszSubject && lstrlenA(lpszSubject))
	{
		LPSTR lpTemp;
		lpTemp=sjis2jis(lpszSubject);
		lstrcpyA(szStr,"Subject: ");
		encode(lpTemp,&szStr[9]);
		GlobalFree(lpTemp);
		lstrcatA(szStr,"\r\n");
		send(s, szStr, lstrlenA(szStr), 0);
	}

	// �����R�[�h��shift-jis�ɐݒ�
	lstrcpyA(szStr, "Content-Type: text/plain; charset=shift-jis;\r\n");
	send(s, szStr, lstrlenA(szStr), 0);
	
	// �{���ƃw�b�_�[�̊�
	lstrcpyA(szStr, "\r\n");
	send(s, szStr, lstrlenA(szStr), 0);
	
	// �{��
	if(lpszBody && lstrlenA(lpszBody))
	{
		char *p,*q;
		int linelen = 0;
		for( p = (char*)lpszBody ; *p != '\0'; p += linelen )
		{
			for( q = p; *q != '\n' && *q != '\0'; q++ );
			//q += 2;
			linelen = q - p + 1; // ���s����������
			lstrcpynA(szStr,p,linelen + 1); //�k������������
			send(s, szStr, lstrlenA(szStr), 0);
		}
	}
	lstrcpyA(szStr, "\r\n.\r\n");
	send(s, szStr, lstrlenA(szStr), 0);
	recv(s, szStrRcv, sizeof(szStrRcv), 0);
	lstrcpyA(szStr, "QUIT\r\n");
	send(s, szStr, lstrlenA(szStr), 0);
	recv(s, szStrRcv, sizeof(szStrRcv), 0);
	closesocket(s);
	WSACleanup();
	return 0;
}

#define GLOBALALLOC(S) reinterpret_cast<LPVOID>(GlobalAlloc(GPTR,(S)))
#define GLOBALFREE(X) {if(X){GlobalFree(reinterpret_cast<HGLOBAL>(X));(X)=NULL;}}

#define HEAPALLOC(S) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(S))
#define HEAPFREE(X) {if(X){HeapFree(GetProcessHeap(),0,reinterpret_cast<LPVOID>(X));(X)=NULL;}}

PSTR *CommandLineToArgvA(const char*lpCmdLine,int*pNumArgs)
{
	int i; PSTR *pArgvA = NULL; int Argc = 0;
	
	PWSTR *pArgvW = NULL;
	PWSTR pM2WBuffer = NULL;
	int *pcchLen = NULL;
	int cchBufferLength = 0;
	
	*pNumArgs = 0;
	
	if((cchBufferLength = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,lpCmdLine,-1,NULL,0)) <= 0)
	{
		goto cleanup;
	}
	
	if((pM2WBuffer = (PWSTR)HEAPALLOC((cchBufferLength+1)*sizeof(WCHAR))) == NULL)
	{
		goto cleanup;
	}
	
	MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,lpCmdLine,-1,pM2WBuffer,cchBufferLength+1);
	pArgvW = CommandLineToArgvW(pM2WBuffer,&Argc);
	
	//�R�}���h���C�����}���`�o�C�g�ɕϊ��������
	//������(�z��pcchLen)�A���������v(cchBufferLength)�����߂�
	if((pcchLen = (int*)HEAPALLOC(sizeof(int)*Argc)) == NULL)
	{
		goto cleanup;
	}
	
	cchBufferLength = 0;
	for(i=0;i<Argc;i++)
	{
		pcchLen[i] = WideCharToMultiByte(CP_ACP,0,pArgvW[i],-1,NULL,0,NULL,NULL);
		cchBufferLength += ++pcchLen[i];
	}
	
	//�R�}���h���C��������̔z����擾���Ċi�[����B
	if((pArgvA = (PSTR*)GLOBALALLOC(cchBufferLength + sizeof(PSTR)*Argc)) == NULL)
	{
		goto cleanup;
	}
	
	for(i=0;i<Argc;i++)
	{
		pArgvA[i] = (i < 1) ? (PSTR)pArgvA + sizeof(PSTR)*Argc : pArgvA[i-1] + pcchLen[i-1];
		WideCharToMultiByte(CP_ACP,0,pArgvW[i],-1,pArgvA[i],pcchLen[i],NULL,NULL);
	}
	
cleanup:
	HEAPFREE(pcchLen);
	HEAPFREE(pM2WBuffer);
	GLOBALFREE(pArgvW);
	*pNumArgs = Argc;
	return pArgvA;
}

void print(LPCSTR lpszText)
{
	HANDLE hStdOut=GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwNumberOfBytesWritten;
	WriteFile(hStdOut, lpszText, lstrlenA(lpszText), &dwNumberOfBytesWritten, 0);
	CloseHandle(hStdOut);
}

VOID Usage()
{
	const char szUsage[]="���[���𑗐M���܂��B\r\n\r\n"
		"SENDMAIL [/m �T�[�o�[] [/f ���o�l] ����[...] [/s ����] [/b �{��]\r\n\r\n"
		"\t�����Ȃ� �w���v��\�����܂��B�u/?�v�Ɠ��͂���̂Ɠ����ł��B\r\n"
		"\t/? �w���v��\�����܂��B�I�v�V��������͂��Ȃ��̂Ɠ����ł��B\r\n"
		"\t/m ���[���T�[�o�[�̐ݒ�����܂��B\r\n"
		"\t/f ���o�l�̃��[���A�h���X�̐ݒ�����܂��B\r\n"
		"\t/s ������ݒ肵�܂��B\r\n"
		"\t/b �{����ݒ肵�܂��B�{���̓p�C�v���g���Ă��ݒ�ł��܂��B\r\n\r\n";
	print(szUsage);
}

BOOL IsMailAddress(const char*pszBuf)
{
	int nBufLen;
	int j;
	int nDotCount;
	int nBgn;
	
	if(!pszBuf)return FALSE;
	nBufLen = lstrlenA(pszBuf);
	
	j = 0;
	if( (pszBuf[j] >= 'a' && pszBuf[j] <= 'z')
		|| (pszBuf[j] >= 'A' && pszBuf[j] <= 'Z')
		|| (pszBuf[j] >= '0' && pszBuf[j] <= '9')
		){
		j++;
	}
	else
	{
		return FALSE;
	}
	while( j < nBufLen - 2 &&
		(
		(pszBuf[j] >= 'a' && pszBuf[j] <= 'z')
		|| (pszBuf[j] >= 'A' && pszBuf[j] <= 'Z')
		|| (pszBuf[j] >= '0' && pszBuf[j] <= '9')
		|| (pszBuf[j] == '.')
		|| (pszBuf[j] == '-')
		|| (pszBuf[j] == '_')
		)
		)
	{
		j++;
	}
	if( j == 0 || j >= nBufLen - 2  )
	{
		return FALSE;
	}
	if( '@' != pszBuf[j] )
	{
		return FALSE;
	}
	j++;
	nDotCount = 0;
	while( 1 )
	{
		nBgn = j;
		while( j < nBufLen &&
			(
			(pszBuf[j] >= 'a' && pszBuf[j] <= 'z')
			|| (pszBuf[j] >= 'A' && pszBuf[j] <= 'Z')
			|| (pszBuf[j] >= '0' && pszBuf[j] <= '9')
			|| (pszBuf[j] == '-')
			|| (pszBuf[j] == '_')
			)
			)
		{
			j++;
		}
		if( j == nBgn )
		{
			return FALSE;
		}
		if( '.' == pszBuf[j] )
		{
			nDotCount++;
			j++;
		}
		else
		{
			if( pszBuf[j] == '\0' )
			{
				if( nDotCount == 0 )
				{
					return FALSE;
				}
				else
				{
					break;
				}
			}
			else
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

BOOL ParseParms(
				int nArgs,
				PCHAR *lplpszArgs
				)
{
	
	int i;
	char*pch;
	
	for (i = 1; i < nArgs; i++)
	{
		if (*(pch = lplpszArgs[i]) == '/')
		{
			while (*++pch)
			{
				switch (*pch)
				{
				case 'm':
				case 'M':
					if (i+1 < nArgs)
					{
						pServer = lplpszArgs[++i];
					}
					else
					{
						Usage();
						return FALSE;
					}
					break;
					
				case 'f':
				case 'F':
					if (i+1 < nArgs)
					{
						if( pFrom != "anonymous" )
						{
							print("���o�l�̃A�h���X�͕����w��ł��܂���B");
							return FALSE;
						}
						else if ( !IsMailAddress(lplpszArgs[i+1]) )
						{
							print("���o�l�̃��[���A�h���X���s���ł��B");
							return FALSE;
						}
						else
						{
							pFrom = lplpszArgs[++i];
						}
					}
					else
					{
						Usage();
						return FALSE;
					}
					break;
					
				case 's':
				case 'S':
					if (i+1 < nArgs)
					{
						pSubject = lplpszArgs[++i];
					}
					else
					{
						Usage();
						return FALSE;
					}
					break;
					
				case 'b':
				case 'B':
					if (i+1 < nArgs)
					{
						pBody = lplpszArgs[++i];
					}
					else
					{
						Usage();
						return FALSE;
					}
					break;
					
				case '?':
				case 'h':
				case 'H':
					Usage();
					return FALSE;
					break;
					
				default:
					Usage();
					return FALSE;
					break;
				}
			}
		}
		else
		{
			
			if( IsMailAddress(lplpszArgs[i]) )
			{
				DWORD dwNewBuffer=lstrlenA(pTo)+lstrlenA(lplpszArgs[i])+2;
				LPSTR pNewTo=(LPSTR)GlobalAlloc(GMEM_FIXED,dwNewBuffer);
				lstrcpyA(pNewTo,pTo);
				GlobalFree(pTo);
				lstrcatA(pNewTo,lplpszArgs[i]);
				lstrcatA(pNewTo,";");
				pTo=pNewTo;
			}
			else
			{
				print("����̃��[���A�h���X���s���ł��B");
				return FALSE;
			}
		}
	}
	
	if(pTo[0]==0)
	{
		Usage();
		return FALSE;
	}
	return TRUE;
}

EXTERN_C void
#ifdef _DEBUG
_mainCRTStartup()
#else
mainCRTStartup()
#endif
{
	int nArgs;
	BOOL bPipe = FALSE;
	char*pPipebody;
	char**lplpszArgs;
	
	HANDLE hFile = GetStdHandle(STD_INPUT_HANDLE);
	
	while ( GetFileType(hFile) == FILE_TYPE_PIPE )
	{
		pPipebody=(char*)GlobalAlloc(GMEM_FIXED,MAX_ATTACHMENT_SIZE+1);
		if(pPipebody==NULL)
		{
			break;
		}
		char*q=pPipebody;
		DWORD dwRead;
		DWORD dwLeft=MAX_ATTACHMENT_SIZE+1;
		do
		{
			if(!ReadFile(hFile,q,dwLeft,&dwRead,0))
			{
				break;
			}
			if(dwRead==0)
			{
				break;
			}
			q+=dwRead;
			dwLeft-=dwRead;
		}
		while(1);
		if(dwLeft==0)
		{
			while(ReadFile(hFile,pPipebody,MAX_ATTACHMENT_SIZE,&dwRead,0)&&dwRead);
			GlobalFree(pPipebody);
			continue;
		}
		bPipe = TRUE;
		break;
	}
	
	lplpszArgs = CommandLineToArgvA(GetCommandLineA(), &nArgs);
	pTo = (char*)GlobalAlloc(GMEM_FIXED,sizeof(char));
	pTo[0]=0;
	
	if(!ParseParms(nArgs,lplpszArgs))
	{
		goto CleanUp;
	}
	
	int nRet;
	
	if(bPipe)
	{
		nRet = sendmail(pServer, pFrom, pTo, pSubject, pPipebody);
	}
	else
	{
		nRet = sendmail(pServer, pFrom, pTo, pSubject, pBody);
	}
	
	switch ( nRet )
	{
	case 0:
		print("���[���𑗐M���܂����B");
		break;
	case -1:
		print("error: �������Ɏ��s���܂����B");
		break;
	case -2:
		print("error: �T�[�o�[��������܂���ł����B");
		break;
	case -3:
		print("error: �\�P�b�g�𐶐��ł��܂�ł����B");
		break;
	case -4:
		print("error: �ڑ��Ɏ��s���܂����B");
		break;
	}
	
CleanUp:
	
	GlobalFree(pTo);
	if(bPipe)
	{
		GlobalFree(pPipebody);
	}
	GlobalFree(lplpszArgs);
	CloseHandle(hFile);
	ExitProcess(0);
}

#ifdef _DEBUG
void main(){_mainCRTStartup();}
#endif

