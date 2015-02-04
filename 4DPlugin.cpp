/* --------------------------------------------------------------------------------
 #
 #	4DPlugin.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : Zip
 #	author : miyako
 #	2015/02/04
 #
 # --------------------------------------------------------------------------------*/


#include "4DPluginAPI.h"
#include "4DPlugin.h"

void PluginMain(PA_long32 selector, PA_PluginParameters params)
{
	try
	{
		PA_long32 pProcNum = selector;
		sLONG_PTR *pResult = (sLONG_PTR *)params->fResult;
		PackagePtr pParams = (PackagePtr)params->fParameters;

		CommandDispatcher(pProcNum, pResult, pParams); 
	}
	catch(...)
	{

	}
}

void CommandDispatcher (PA_long32 pProcNum, sLONG_PTR *pResult, PackagePtr pParams)
{
	switch(pProcNum)
	{
// --- Zip

		case 1 :
			Unzip(pResult, pParams);
			break;

		case 2 :
			Zip(pResult, pParams);
			break;

	}
}

// -------------------------------------- Zip -------------------------------------

void Unzip(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_TEXT Param2;
	C_TEXT Param3;
	C_LONGINT Param4;
	C_LONGINT returnValue;

	Param1.fromParamAtIndex(pParams, 1);
	Param2.fromParamAtIndex(pParams, 2);
	Param3.fromParamAtIndex(pParams, 3);
	Param4.fromParamAtIndex(pParams, 4);

    //pass
    CUTF8String password;
    Param3.copyUTF8String(&password);
    
    //src, dst
#if VERSIONMAC
    CUTF8String input_path, output_path;
    Param1.copyPath(&input_path);
    Param2.copyPath(&output_path);
    const char *input = (const char*)input_path.c_str();
    const char *output = (const char*)output_path.c_str();
#else
    const wchar_t *input = (const wchar_t*)Param1.getUTF16StringPtr();
    const wchar_t *output = (const wchar_t*)Param2.getUTF16StringPtr();    
#endif  
    
    //ignore_dot
    unsigned int ignore_dot = Param4.getIntValue();  
    
    unzFile hUnzip = unzOpen64(input);
    
    if (hUnzip){
        
        returnValue.setIntValue(1);
        
        unz_file_info64 fileInfo;
        
        std::vector<uint8_t> szConFilename(PATH_MAX);
        
        relative_path_t relative_path;
        absolute_path_t sub_path, absolute_path;
        
        do {
            
            PA_YieldAbsolute();
            
            if (unzGetCurrentFileInfo64(hUnzip, &fileInfo, (char *)&szConFilename[0], PATH_MAX, NULL, 0, NULL, 0) != UNZ_OK){
                returnValue.setIntValue(0);
                break;
            }
            
            get_relative_path(&szConFilename[0], sub_path, relative_path);

            absolute_path = output;
            absolute_path+= folder_separator + sub_path;
            
            if(relative_path.size() > 1){
                if( !ignore_dot || ((relative_path.at(0) != '.') && relative_path.find("/.") == std::string::npos)){
                    create_parent_folder(absolute_path);
                    
                    if(relative_path.at(relative_path.size() - 1) == folder_separator){
                        create_folder(absolute_path);
                    }

                    if(password.length()){
                        if(unzOpenCurrentFilePassword(hUnzip, (const char *)password.c_str()) != UNZ_OK){
                            returnValue.setIntValue(0);
                            break;
                        }  
                    }else{ 
                        if(unzOpenCurrentFile(hUnzip) != UNZ_OK){
                            returnValue.setIntValue(0);
                            break;
                        }
                    }
                    
                    std::ofstream ofs(absolute_path.c_str(), std::ios::out|std::ios::binary);
                    
                    if(ofs.is_open()){
                        
                        std::vector<uint8_t> buf(BUFFER_SIZE);
                        std::streamsize size;
                        
                        while ((size = unzReadCurrentFile(hUnzip, &buf[0], BUFFER_SIZE)) > 0){
                            PA_YieldAbsolute();
                            ofs.write((const char *)&buf[0], size);
                        }
                        
                        ofs.close();
                        
                    }
                    
                }   
                
            }

            unzCloseCurrentFile(hUnzip);
            
        } while (unzGoToNextFile(hUnzip) != UNZ_END_OF_LIST_OF_FILE);
        
        unzClose(hUnzip);	
    }     
    
	returnValue.setReturn(pResult);
}

#ifdef WIN32

using namespace std;

int wcs_to_utf8(wstring& wstr, string& str){
   
    int error = 0; 
    
    int len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
    if(len){
        vector<char> buf(len + 1);
        if(WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wstr.c_str(), wstr.length(), (LPSTR)&buf[0], len, NULL, NULL)){
            str = string((const char *)&buf[0]);
        }
    }else{
        str = string((const char *)"");
        error = -1;
    }
    
    return error;   
    
}

int utf8_to_wcs(string& str, wstring& wstr)
{
   
    int error = 0; 
    
    int len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), str.length(), NULL, 0);
    if(len){
        vector<char> buf((len + 1) * sizeof(wchar_t));
        if(MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), str.length(), (LPWSTR)&buf[0], len)){
            wstr = wstring((const wchar_t *)&buf[0]);
        }
    }else{
        wstr = wstring((const wchar_t *)L"");
        error = -1;
    }
    
    return error;   
    
}

void unescape_path(string &path)
{
    wstring wpath;
    utf8_to_wcs(path, wpath);
    unescape_path(wpath);
    wcs_to_utf8(wpath, path);
}

void escape_path(string &path)
{
    wstring wpath;
    utf8_to_wcs(path, wpath);
    escape_path(wpath);
    wcs_to_utf8(wpath, path);
}

void unescape_path(wstring &path)
{
    for (unsigned int i = 0; i < path.size(); ++i)
        if (path.at(i) == '/')
            path.at(i) = L'\\';
}

void escape_path(wstring &path)
{
    for (unsigned int i = 0; i < path.size(); ++i)
        if (path.at(i) == '\\')
            path.at(i) = L'/';
}

void get_subpaths(wstring& path,
                  absolute_paths_t *absolute_paths,
                  relative_paths_t *relative_paths,
				  relative_path_t& folder_name,
                  int ignore_dot,
                  size_t absolutePathOffset = 0)
{
    
    WIN32_FIND_DATA find;	
    
    HANDLE h = FindFirstFile(path.c_str(), &find);
    
    absolute_path_t absolute_path;
    relative_path_t relative_path;

    if(h != INVALID_HANDLE_VALUE){
        
        do {
            
            PA_YieldAbsolute();
            
            wstring sub_path = find.cFileName;	
            
            if((!wcscmp(sub_path.c_str(), L"..")) || (!wcscmp(sub_path.c_str(), L".")))
                continue;		
            
            if(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
                
                if(!absolutePathOffset){
                    //top level is folder
                    absolutePathOffset = path.size() - 1;

					wcs_to_utf8(sub_path + L"/", folder_name);	

                    get_subpaths(path + L"\\*", 
                                 absolute_paths, 		
                                 relative_paths,       
                                 folder_name,
                                 ignore_dot, 
                                 absolutePathOffset);
                    
                }else{
                    //is sub-folder
                    absolute_path = path + sub_path; 
                    
                    wstring base_path = path.substr(0, path.size() - 1).substr(absolutePathOffset + 2);
                    base_path += sub_path;
                    base_path += L"\\";
                    escape_path(base_path);
                    wcs_to_utf8(base_path, relative_path);
					relative_path = folder_name + relative_path;

                    if(!ignore_dot || ((relative_path.at(0) != '.') && relative_path.find("/.") == string::npos)){
                     
                        absolute_paths->push_back(absolute_path);
                        relative_paths->push_back(relative_path);
                        
                        get_subpaths(path.substr(0, path.size() - 1)  + sub_path + L"\\*", 
                                     absolute_paths, 
                                     relative_paths, 
									 folder_name,
                                     ignore_dot,
                                     absolutePathOffset);
                        
                    }
  
                }
                
            }else{
                
                if(!absolutePathOffset){
                    // (over-ride ignore_dot, this is top level)
                    absolute_path = path + sub_path;  
                    
                    escape_path(sub_path);
                    wcs_to_utf8(sub_path, relative_path);

                    absolute_paths->push_back(absolute_path);
                    relative_paths->push_back(relative_path);
                    
                }else{
                    
					wstring base_path = path.substr(0, path.size() - 1);
                    absolute_path = base_path + sub_path;  
                    
                    sub_path = base_path.substr(absolutePathOffset + 2) + sub_path;
                    escape_path(sub_path);
                    wcs_to_utf8(sub_path, relative_path);
					relative_path = folder_name + relative_path;

                    if(!ignore_dot || ((relative_path.at(0) != '.') && relative_path.find("/.") == string::npos)){
                        absolute_paths->push_back(absolute_path);
                        relative_paths->push_back(relative_path);
                    }
  
                }
                
            }		
            
        } while (FindNextFile(h, &find));
        
		if(!absolute_paths->size() && absolutePathOffset){
			wstring base_path = path.substr(0, path.size() - 1);
			relative_paths->push_back(folder_name);	
                absolute_paths->push_back(base_path);
		}

        FindClose(h);
        
    }    
    
}
#endif

void get_subpaths(C_TEXT& Param, 
                  relative_paths_t *relative_paths, 
                  absolute_paths_t *absolute_paths, 
                  int ignore_dot){
    
    relative_paths->clear();
    absolute_paths->clear();
    
#if VERSIONMAC
    
    std::string spath;
    copy_path(Param, spath);
    NSString *path = (NSString *)CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, spath.c_str());	
    
    //semantically the same string but the result from subpathsOfDirectoryAtPath is wrong
    //NSString *path = Param.copyPath();
    
    NSFileManager *fm = [[NSFileManager alloc]init];
    
    BOOL isDirectory = YES;
    
    if([fm fileExistsAtPath:path isDirectory:&isDirectory]){
        
        if(isDirectory){
            
            NSString *folderName = [[path lastPathComponent]stringByAppendingString:@"/"];
            NSArray *paths = [fm subpathsOfDirectoryAtPath:path error:NULL];
            
            //a folder with contents
            for(NSUInteger i = 0; i < [paths count]; i++){
                
                PA_YieldAbsolute();
                
                NSString *itemPath = [paths objectAtIndex:i];   
                NSString *itemFullPath = [path stringByAppendingPathComponent:itemPath];  
                
                if([fm fileExistsAtPath:itemFullPath isDirectory:&isDirectory]){
                    
                    if(isDirectory)
                        itemPath = [itemPath stringByAppendingString:@"/"];
                    
                    absolute_path_t absolute_path = [itemFullPath UTF8String];
                    relative_path_t relative_path = [[folderName stringByAppendingString:itemPath]UTF8String];
                    
                    if(!ignore_dot || ((relative_path.at(0) != '.') && relative_path.find("/.") == std::string::npos)){
                        relative_paths->push_back(relative_path);	
                        absolute_paths->push_back(absolute_path);
                    }

                }				
                
            }
            
            //an empty folder (over-ride ignore_dot, this is top level)
            if(!relative_paths->size()){
                relative_paths->push_back([folderName UTF8String]);	
                absolute_paths->push_back(spath);
            }
            
        }else{	
            //a file (over-ride ignore_dot, this is top level)
            relative_paths->push_back(std::string([[path lastPathComponent]UTF8String]));
            absolute_paths->push_back(spath);
        }
        
    }
    
    [path release];	
    [fm release];	
    
#else
    
    std::wstring path = std::wstring((wchar_t *)Param.getUTF16StringPtr());
	relative_path_t folder_name;
    get_subpaths(path, absolute_paths, relative_paths, folder_name, ignore_dot);
    
#endif	
}

void Zip(sLONG_PTR *pResult, PackagePtr pParams)
{

    C_TEXT Param1;
    C_TEXT Param2;
    C_TEXT Param3;
    C_LONGINT Param4;
    C_LONGINT Param5;
    C_LONGINT returnValue;
    
    Param1.fromParamAtIndex(pParams, 1);//src
    Param2.fromParamAtIndex(pParams, 2);//dst
    Param3.fromParamAtIndex(pParams, 3);//pass
    Param4.fromParamAtIndex(pParams, 4);//level
    Param5.fromParamAtIndex(pParams, 5);//ignore_dot    
  
    //src
    absolute_paths_t absolute_paths;
    relative_paths_t relative_paths;
    relative_path_t input_file_name;
    
    //pass
    CUTF8String password;
    Param3.copyUTF8String(&password);
    
    //dst
#if VERSIONMAC
    CUTF8String output_path;
    Param2.copyPath(&output_path);
    const char *output = (const char*)output_path.c_str();
#else
    const wchar_t *output = (const wchar_t*)Param2.getUTF16StringPtr();
#endif  
    
    //level
    unsigned int level = Param4.getIntValue();
    if(!level){
        level = Z_DEFAULT_COMPRESSION; 
    }else if (level > 10){
        level = 9;
    }
    
    //ignore_dot
    unsigned int ignore_dot = Param5.getIntValue();
    
    unsigned long CRC; 
    
    zipFile hZip = zipOpen64(output, APPEND_STATUS_CREATE);
            
    if(hZip){
        
        returnValue.setIntValue(1);
        
        get_subpaths(Param1, &relative_paths, &absolute_paths, ignore_dot);
                
        zip_fileinfo zi;
        
        time_t currentTime;
        time(&currentTime);
        
        struct tm *tm;
        tm=localtime(&currentTime);
        
        zi.tmz_date.tm_sec=tm->tm_sec;
        zi.tmz_date.tm_min=tm->tm_min;
        zi.tmz_date.tm_hour=tm->tm_hour;
        zi.tmz_date.tm_mday=tm->tm_mday;
        zi.tmz_date.tm_mon=tm->tm_mon;
        zi.tmz_date.tm_year=tm->tm_year;
        zi.external_fa = 0;
        zi.internal_fa = 0;
        zi.dosDate = 0;
        zi.internal_fa = zi.external_fa = 0;
        
        for (unsigned int i = 0; i < relative_paths.size(); ++i) {
            
            PA_YieldAbsolute();
            
            relative_path_t relative_path = relative_paths.at(i);
            absolute_path_t absolute_path = absolute_paths.at(i); 
            
            if(password.length()){
                
                CRC = crc32(0L, Z_NULL, 0);
                
                std::ifstream ifs_crc(absolute_path.c_str(), std::ios::in|std::ios::binary);
                
                if(ifs_crc.is_open()){
                    
                    std::vector<uint8_t> buf(BUFFER_SIZE);
                    
                    while(ifs_crc.good()){
                        
                        PA_YieldAbsolute();
                        
                        ifs_crc.read((char *)&buf[0], BUFFER_SIZE);
                        
                        CRC = crc32(CRC, (const Bytef *)&buf[0], ifs_crc.gcount());
                        
                    }
                    
                    ifs_crc.close();
                    
                }
                
                if(zipOpenNewFileInZip3_64(hZip,
                                           relative_path.c_str(),
                                           &zi,
                                           NULL, 0,
                                           NULL, 0,
                                           NULL,
                                           Z_DEFLATED,
                                           level,
                                           0, 15, 8, Z_DEFAULT_STRATEGY,
                                           (const char *)password.c_str(), CRC, 1) != 0){
                    returnValue.setIntValue(0);
                    break;
                }
                
            }else{
                
                if(zipOpenNewFileInZip64(hZip,
                                         relative_path.c_str(),
                                         &zi,
                                         NULL, 0,
                                         NULL, 0,
                                         NULL,
                                         Z_DEFLATED,
                                         level,
                                         0) != 0){
                    returnValue.setIntValue(0);
                    break;
                }
                
            }
            
            std::ifstream ifs(absolute_path.c_str(), std::ios::in|std::ios::binary);
            
            if(ifs.is_open()){
                
                std::vector<uint8_t> buf(BUFFER_SIZE);
                
                while(ifs.good()){
                    PA_YieldAbsolute();
                    ifs.read((char *)&buf[0], BUFFER_SIZE);
                    zipWriteInFileInZip(hZip, (char *)&buf[0], ifs.gcount()); 
                }
                
                ifs.close();
                
            }
            
            zipCloseFileInZip(hZip);
            
        }
        
        zipClose(hZip, NULL);
        
    }
    
    returnValue.setReturn(pResult);
}

void get_relative_path(void *p, absolute_path_t& sub_path, relative_path_t &relative_path){
#if VERSIONWIN
    relative_path = relative_path_t((const char *)p);
    std::string path = relative_path; 
    unescape_path(path);
    utf8_to_wcs(path, sub_path);
#else
    relative_path = absolute_path_t((const char *)p);
    sub_path = relative_path;
#endif    
}

bool create_folder(absolute_path_t& absolute_path){
    
    bool success = false;
    
#if VERSIONMAC
    NSString *path = (NSString *)CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, absolute_path.c_str());
    NSFileManager *fm = [[NSFileManager alloc]init];
    success = [fm createDirectoryAtPath:path 
            withIntermediateDirectories:YES
                             attributes:nil
                                  error:NULL];	
    [path release];					
    [fm release];			
#else
    success = SHCreateDirectory(NULL, (PCWSTR)absolute_path.c_str());				
#endif	
    
    return success;
}

void create_parent_folder(absolute_path_t& absolute_path){
#if VERSIONMAC
    NSString *filePath = (NSString *)CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, absolute_path.c_str());
    absolute_path_t folderPath = absolute_path_t([[filePath stringByDeletingLastPathComponent]fileSystemRepresentation]);
    create_folder(folderPath);
    [filePath release];
#else	
    wchar_t	fDrive[_MAX_DRIVE],
    fDir[_MAX_DIR],
    fName[_MAX_FNAME],
    fExt[_MAX_EXT];
    _wsplitpath_s(absolute_path.c_str(), fDrive, fDir, fName, fExt);	
    absolute_path_t folderPath = fDrive;
    folderPath += fDir;
    create_folder(folderPath);
#endif	
}

void copy_path(C_TEXT& t, absolute_path_t& p){
#if VERSIONMAC	
    NSString *str = t.copyUTF16String();
    NSURL *u = (NSURL *)CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)str, kCFURLHFSPathStyle, false);
    
    if(u){
        NSString *path = (NSString *)CFURLCopyFileSystemPath((CFURLRef)u, kCFURLPOSIXPathStyle);
        CFIndex size = CFStringGetMaximumSizeOfFileSystemRepresentation((CFStringRef)path);
        std::vector<uint8_t> buf(size);		
        [path getFileSystemRepresentation:(char *)&buf[0] maxLength:size];
        p = std::string((char *)&buf[0]);
        [path release];
        [u release];
    }	
    
    [str release];
    
#else
    p = std::wstring((wchar_t *)t.getUTF16StringPtr());
    
    if(p.at(p.size() - 1) == L'\\')
        p = p.substr(0, p.size() - 1);
#endif	
}