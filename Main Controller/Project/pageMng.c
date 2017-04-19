#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"
#include "stm32f10x_flash.h"
#include "typedef.h"
#include "common.h"
#include "ff.h"
#include "diskio.h"
#include "dataStruct.h"
#include "pageMng.h"
#include "c_func.h"

DIR Dir_P;
FIL Fil_P;

extern char imgDir[5];
extern char pageDir[4];
extern char fontDir[4];
extern PAGE* page;
extern HW_CONFIG hwConfig;

//============================================
bool checkImgExist(u16 imgID)
{	
	u16 fileCount = 0;
	u16 i = 0;
	u16* fileList;
	bool result = false;

	fileCount = countFile(imgDir);
	if(fileCount)
	{
		fileList = (u16*)m_calloc(fileCount,sizeof(u16));
		if(fileList != NULL)
		{
			fileCount = getListFile(imgDir,fileList);
			if(fileCount)
			{
				for(i = 0;i<fileCount;i++)
				{
					if(fileList[i] == imgID)
					{
						result = true;
						break;
					}
				}
			}
		}
		m_free(fileList);
	}

	return result;
}

//============================================
PAGE* loadPage(u16 pageID)
{
	FRESULT rc;
	UINT br;
	char name[20] = "";
	u16 i = 0,j = 0;
	PAGE* page = NULL;
	u16 imgID = 0;
	u32 dataSize = 0;
	F_RETURN result = F_SUCCESS;

	// Read page's information
	rc = f_opendir(&Dir_P,pageDir);
	if(rc == FR_OK)
	{
		convert_name(pageID,name,FILE_PAGE);
		rc = f_open(&Fil_P, name, FA_READ);	
	}
	
	if(rc == FR_OK)
	{
		page = (PAGE*)m_calloc(1,sizeof(PAGE));	
		rc = f_read(&Fil_P,&page->header,PAGE_HEADER_LENGTH,&br);
		if(br == PAGE_HEADER_LENGTH)
		{
			page->popup = (POPUP*)m_calloc(page->header.itemCount,sizeof(POPUP));			
			for(i = 0;i<page->header.itemCount;i++)
			{
				dataSize = 0;
				//page->popup[i].Info.data = NULL;
				rc = f_read(&Fil_P,&page->popup[i].Info.header,POPUP_HEADER_LENGTH,&br);
				if(br == POPUP_HEADER_LENGTH)
				{
					page->popup[i].Info.header.isUpdate = false;
					switch(page->popup[i].Info.header.dataType)
					{
						case POPUP_PLAYLIST:
							{
								PLAYLIST_ITEM* playlist;

								page->popup[i].Info.header.activeItem = 0;								
								dataSize = sizeof(PLAYLIST_HEADER);
								page->popup[i].Info.data = (PLAYLIST_ITEM*)m_calloc(page->popup[i].Info.header.itemCount,sizeof(PLAYLIST_ITEM));
								playlist = (PLAYLIST_ITEM*)page->popup[i].Info.data;
								for(j = 0;j<page->popup[i].Info.header.itemCount;j++)
								{
									rc = f_read(&Fil_P,&playlist[j].header,dataSize,&br);
									if(br == dataSize)
									{
										switch(playlist[j].header.dataType)
										{
											case PLAYLIST_IMAGE:											
												playlist[j].data = (u8*)m_calloc(1,sizeof(u16));
												rc = f_read(&Fil_P,playlist[j].data,sizeof(u16),&br);
												playlist[j].isLive = true;
												playlist[j].TTL_count = playlist[j].header.TTL;
												memcpy(&imgID,playlist[j].data,2);												
												if(!checkImgExist(imgID))
												{
													result = F_IMG_NOT_EXIST;
												}
												break;
											case PLAYLIST_TEXT:
											case PLAYLIST_CLOCK:
											case PLAYLIST_TEMP:
												playlist[j].data = (u8*)m_calloc(playlist[j].header.length,sizeof(u8));
												rc = f_read(&Fil_P,playlist[j].data,playlist[j].header.length,&br);
												playlist[j].isLive = true;
												playlist[j].TTL_count = playlist[j].header.TTL;
												break;
											default:
												break;
										}
									}
									if(result != F_SUCCESS)
										break;
								}		
								page->popup[i].Info.header.countDown = playlist[page->popup[i].Info.header.activeItem].header.TTS;
							}
							break;						
						default:
							break;
					}
				}
				else
					result = F_ERROR;
				
				if(result != F_SUCCESS)
					break;
			}
		}
		else
			result = F_ERROR;
		
		if(result != F_SUCCESS)
		{
			freePageResource(page);
			page = NULL;
		}
	}
	
	f_close(&Fil_P);

	return page;
}

//============================================
F_RETURN savePage(PAGE* page)
{
	char name[20] = "";								
	F_RETURN result = F_SUCCESS;
	FRESULT rc;
	UINT bw;
	u16 i = 0,j = 0;
	u32 dataSize = 0;
						
	rc = f_opendir(&Dir_P,pageDir);
	if(rc == FR_OK)
	{
		convert_name(page->header.ID,name,FILE_PAGE);
		rc = f_unlink(name);
		rc = f_open(&Fil_P,&name[0],FA_CREATE_ALWAYS | FA_WRITE);
	}
	if(rc == FR_OK)
		rc = f_write(&Fil_P,&page[0],PAGE_HEADER_LENGTH,&bw);
	if(rc == FR_OK)
	{
		for(i = 0;i<page->header.itemCount;i++)
		{
			dataSize = 0;
			rc = f_write(&Fil_P,&page->popup[i],POPUP_HEADER_LENGTH,&bw);
			switch(page->popup[i].Info.header.dataType)
			{
				case POPUP_PLAYLIST:
					{
						PLAYLIST_ITEM* playlist;
						
						dataSize = sizeof(PLAYLIST_HEADER);											
						playlist = (PLAYLIST_ITEM*)page->popup[i].Info.data;
						for(j = 0;j<page->popup[i].Info.header.itemCount;j++)
						{
							rc = f_write(&Fil_P,&playlist[j].header,dataSize,&bw);							
							if(bw == dataSize)
							{
								switch(playlist[j].header.dataType)
								{
									case PLAYLIST_IMAGE:
										rc = f_write(&Fil_P,playlist[j].data,sizeof(u16),&bw);
										break;
									case PLAYLIST_TEXT:		
									case PLAYLIST_CLOCK:
									case PLAYLIST_TEMP:
										rc = f_write(&Fil_P,playlist[j].data,playlist[j].header.length,&bw);									
										break;
									default:
										break;
								}
							}
						}				
					}
					break;			
				default:
					break;
			}
		}
	}
	f_close(&Fil_P);
	if(rc != FR_OK)
	{
		result = F_SDC_ERR;
	}

	return result;
}

//============================================
void freePageResource(PAGE* page)
{
	u16 i = 0,j = 0;
	
	if(page != NULL)
	{
		for(i = 0;i<page->header.itemCount;i++)
		{
			if(page->popup[i].Info.header.dataType == POPUP_PLAYLIST)
			{
				PLAYLIST_ITEM* playlist = (PLAYLIST_ITEM*)page->popup[i].Info.data;
				for(j = 0;j<page->popup[i].Info.header.itemCount;j++)
					m_free(playlist[j].data);	
				m_free(page->popup[i].Info.data);
			}			
		}
		m_free(page->popup);
		m_free(page);
		page = NULL;
	}
}

//============================================
void freePopup(POPUP* popup)
{
	int i = 0;
	if(popup->Info.header.dataType == POPUP_PLAYLIST)
	{
		PLAYLIST_ITEM* playlist = (PLAYLIST_ITEM*)popup->Info.data;
		for(i = 0;i<popup->Info.header.itemCount;i++)
			m_free(playlist[i].data);	
		m_free(popup->Info.data);
	}	
}
//============================================
F_RETURN createPopup(u16 pageID,u16 popupID,u8* popupData)
{
	u16 popupIndex = 0;
	PAGE* pageTemp = NULL;
	char name[20] = "";		
	FRESULT rc;
	u16 index = 0,i = 0,j = 0;
	u32 dataSize = 0;
	u16 imgID = 0;
	F_RETURN result = F_SUCCESS;

	index = 0;
	
	if(pageTemp != NULL)
		freePageResource(pageTemp);
	
	pageTemp = loadPage(pageID);
	
	if(pageTemp != NULL)
	{
		// Check is popup ID exist
		for(i = 0;i<pageTemp->header.itemCount;i++)
		{
			if(pageTemp->popup[i].Info.header.ID == popupID)
			{
				result = F_DUPLICATE;
				break;
			}
		}
		if(result == F_SUCCESS)
		{
			pageTemp->header.itemCount +=1;
			pageTemp->popup = realloc(pageTemp->popup,pageTemp->header.itemCount*sizeof(POPUP));
			popupIndex = pageTemp->header.itemCount-1;
			memcpy(&pageTemp->popup[popupIndex],&popupData[index],POPUP_HEADER_LENGTH);
			index += POPUP_HEADER_LENGTH;
			dataSize = 0;
			result = verifyPopupSize(pageTemp->popup[popupIndex]);
		}
		if(result == F_SUCCESS)
		{
			switch(pageTemp->popup[popupIndex].Info.header.dataType)
			{
				case POPUP_PLAYLIST:
					{
						PLAYLIST_ITEM* playlist;
						dataSize = sizeof(PLAYLIST_HEADER);
						pageTemp->popup[popupIndex].Info.data = (PLAYLIST_ITEM*)m_calloc(pageTemp->popup[popupIndex].Info.header.itemCount,sizeof(PLAYLIST_ITEM));
						playlist = (PLAYLIST_ITEM*)pageTemp->popup[popupIndex].Info.data;
						for(j = 0;j<pageTemp->popup[popupIndex].Info.header.itemCount;j++)
						{
							memcpy(&playlist[j].header,&popupData[index],dataSize);
							index += dataSize;
							switch(playlist[j].header.dataType)
							{
								case PLAYLIST_IMAGE:
									playlist[j].data = (u8*)m_calloc(1,sizeof(u16));
									memcpy(playlist[j].data,&popupData[index],sizeof(u16));
									memcpy(&imgID,&popupData[index],sizeof(u16));
									index += sizeof(u16);
									if(!checkImgExist(imgID))
									{
										result = F_IMG_NOT_EXIST;
									}
									break;
								case PLAYLIST_TEXT:
								case PLAYLIST_CLOCK:
								case PLAYLIST_TEMP:
									playlist[j].data = (u8*)m_calloc(playlist[j].header.length,sizeof(u8));
									memcpy(playlist[j].data,&popupData[index],playlist[j].header.length);
									index += playlist[j].header.length;
									break;
							}
							if(result != F_SUCCESS)
								break;
						}							
					}
					break;				
				default:
					pageTemp->popup[popupIndex].Info.data = NULL;
					break;
			}
			if(result == F_SUCCESS)
			{
				rc = f_opendir(&Dir_P,pageDir);
				if(rc == FR_OK)
				{
					convert_name(pageID,name,FILE_PAGE);
					rc = f_unlink(name);
				}
				if(rc == FR_OK)
				{
					savePage(pageTemp);				
				}
			}
		}
	}
	else
		result = F_ERROR;

	if(pageTemp != NULL)
		freePageResource(pageTemp);
	
	return result;
}

//============================================
F_RETURN updatePopup(u16 pageID,u16 popupID,u8* popupData)
{
	PAGE* pageTemp = NULL;
	POPUP popupTemp;
	u16 index = 0;
	char name[20] = "";
	FRESULT rc;
	u16 i = 0,j = 0;
	u32 dataSize = 0;
	u16 imgID = 0;
	F_RETURN result = F_SUCCESS;
	
	if(pageTemp != NULL)
		freePageResource(pageTemp);
	
	pageTemp = loadPage(pageID);
	if(pageTemp != NULL)
	{
		// Replace popup data
		for(i = 0;i<pageTemp->header.itemCount;i++)
		{
			result = F_ERROR;
			if(pageTemp->popup[i].Info.header.ID == popupID)
			{
				memcpy(&popupTemp,&popupData[index],POPUP_HEADER_LENGTH);
				if(popupTemp.Info.header.ID != popupID)
				{
					result = F_INVALID_ID;
					break;
				}
				freePopup(&pageTemp->popup[i]);
				memcpy(&pageTemp->popup[i],&popupData[index],POPUP_HEADER_LENGTH);
				index += POPUP_HEADER_LENGTH;
				dataSize = 0;
				result = verifyPopupSize(pageTemp->popup[i]);
				if(result == F_SUCCESS)
				{
					switch(pageTemp->popup[i].Info.header.dataType)
					{
						case POPUP_PLAYLIST:
							{
								PLAYLIST_ITEM* playlist;
								
								dataSize = sizeof(PLAYLIST_HEADER);
								pageTemp->popup[i].Info.data = (PLAYLIST_ITEM*)m_calloc(pageTemp->popup[i].Info.header.itemCount,sizeof(PLAYLIST_ITEM));
								playlist = (PLAYLIST_ITEM*)pageTemp->popup[i].Info.data;
								for(j = 0;j<pageTemp->popup[i].Info.header.itemCount;j++)
								{
									memcpy(&playlist[j].header,&popupData[index],dataSize);
									index += dataSize;
									switch(playlist[j].header.dataType)
									{
										case PLAYLIST_IMAGE:	
											playlist[j].data = (u8*)m_calloc(1,sizeof(u16));
											memcpy(playlist[j].data,&popupData[index],sizeof(u16));
											memcpy(&imgID,&popupData[index],sizeof(u16));
											index += sizeof(u16);
											if(!checkImgExist(imgID))
											{
												result = F_IMG_NOT_EXIST;
											}
											break;
										case PLAYLIST_TEXT:
										case PLAYLIST_CLOCK:
										case PLAYLIST_TEMP:
											playlist[j].data = (u8*)m_calloc(playlist[j].header.length,sizeof(u8));
											memcpy(playlist[j].data,&popupData[index],playlist[j].header.length);
											index += playlist[j].header.length;
											break;
									}
								}							
							}
							break;					
						default:
							result = F_ERROR;
							break;
					}
//					result = F_SUCCESS;
//					break;
				}
				break;
			}
		}
		if(result == F_SUCCESS)
		{
			rc = f_opendir(&Dir_P,pageDir);
			if(rc == FR_OK)
			{
				convert_name(pageID,name,FILE_PAGE);
				rc = f_unlink(name);
			}
			if(rc == FR_OK)
			{
				savePage(pageTemp);								
			}
		}
	}
	else
		result = F_ERROR;

	if(pageTemp != NULL)
		freePageResource(pageTemp);
	
	return result;
}

//============================================
F_RETURN deletePopup(u16 pageID,u16 popupID)
{
	PAGE* pageTemp = NULL;
	char name[20] = "";		
	POPUP* popupTemp = NULL;
	u16 index = 0;
	FRESULT rc;
	u16 i = 0,j = 0;
	F_RETURN result = F_SUCCESS;
	
	if(pageTemp != NULL)
		freePageResource(pageTemp);
	pageTemp = loadPage(pageID);
	if(pageTemp != NULL)
	{
		result = F_ERROR;
		// Copy all popup header to buffer
		popupTemp = (POPUP*)m_calloc(pageTemp->header.itemCount,sizeof(POPUP));
		memcpy(popupTemp,pageTemp->popup,pageTemp->header.itemCount*sizeof(POPUP));
		for(i = 0;i<pageTemp->header.itemCount;i++)
		{
			popupTemp[i].Info.data = pageTemp->popup[i].Info.data;
		}
		
		// Free resource used by popup need to remove
		for(i = 0;i<pageTemp->header.itemCount;i++)
		{
			if(pageTemp->popup[i].Info.header.ID == popupID)
			{
				PLAYLIST_ITEM* playlist = (PLAYLIST_ITEM*)pageTemp->popup[i].Info.data;
				
				for(j = 0;j<pageTemp->popup[i].Info.header.itemCount;j++)
				{
					free(playlist[j].data);
				}
				free(pageTemp->popup[i].Info.data);
				// Free popup header resource
				free(pageTemp->popup);
				// Re-allocate popup resource to fit new size
				pageTemp->header.itemCount -=1;
				pageTemp->popup = (POPUP*)m_calloc(pageTemp->header.itemCount,sizeof(POPUP));
				// Copy all popup header from buffer to page resource,except deleting popup
				index = 0;
				for(j = 0;j<(pageTemp->header.itemCount+1);j++)
				{				
					if(popupTemp[j].Info.header.ID != popupID)
					{
						memcpy(&pageTemp->popup[index],&popupTemp[j],sizeof(POPUP));
						pageTemp->popup[index].Info.data = popupTemp[j].Info.data;
						index ++;
					}
				}
				rc = f_opendir(&Dir_P,pageDir);
				if(rc == FR_OK)
				{
					convert_name(pageID,name,FILE_PAGE);
					rc = f_unlink(name);
				}
				if(rc == FR_OK)
				{
					savePage(pageTemp);	
				}
				result = F_SUCCESS;
				break;
			}
		}
	}
	else 
		result = F_ERROR;
	if(pageTemp != NULL)
		freePageResource(pageTemp);
	if(popupTemp != NULL)
		free(popupTemp);	
	
	return result;
}

//============================================
F_RETURN createPage(u8* pageData)
{
	PAGE* pageTemp = NULL;
	u16 i = 0,j = 0;
	u16 index= 0;
	u32 dataSize = 0;
	u16 imgID = 0;
	F_RETURN result = F_SUCCESS;

	index = 0;
	if(pageTemp != NULL)
		freePageResource(pageTemp);
		
	pageTemp = (PAGE*)m_calloc(1,sizeof(PAGE));
	memcpy(&pageTemp[0],&pageData[index],PAGE_HEADER_LENGTH);
	index += PAGE_HEADER_LENGTH;
	if(pageTemp->header.itemCount > 0)
	{
		pageTemp->popup = (POPUP*)m_calloc(pageTemp->header.itemCount,sizeof(POPUP));

		for(i = 0;i<pageTemp->header.itemCount;i++)
		{
			dataSize = 0;
			memcpy(&pageTemp->popup[i],&pageData[index],POPUP_HEADER_LENGTH);
			index += POPUP_HEADER_LENGTH;
			// Check is popup ID duplicate
			for(j = 0;j<i;j++)
			{
				if(pageTemp->popup[j].Info.header.ID == pageTemp->popup[i].Info.header.ID)
				{
					result = F_ERROR;
					break;
				}
			}			
			if(result == F_SUCCESS)
				result = verifyPopupHeader(pageTemp->popup[i]);
			if(result == F_SUCCESS)
			{
				switch(pageTemp->popup[i].Info.header.dataType)
				{
					case POPUP_PLAYLIST:
						{
							PLAYLIST_ITEM* playlist;
							dataSize = sizeof(PLAYLIST_HEADER);
							pageTemp->popup[i].Info.data = (PLAYLIST_ITEM*)m_calloc(pageTemp->popup[i].Info.header.itemCount,sizeof(PLAYLIST_ITEM));
							playlist = (PLAYLIST_ITEM*)pageTemp->popup[i].Info.data;
							for(j = 0;j<pageTemp->popup[i].Info.header.itemCount;j++)
							{
								memcpy(&playlist[j].header,&pageData[index],dataSize);
								index += dataSize;
								switch(playlist[j].header.dataType)
								{
									case PLAYLIST_IMAGE:
										playlist[j].data = (u8*)m_calloc(1,2);
										memcpy(playlist[j].data,&pageData[index],2);
										memcpy(&imgID,playlist[j].data,2);
										index += 2;
										if(!checkImgExist(imgID))
										{
											result = F_IMG_NOT_EXIST;
										}
										break;
									case PLAYLIST_TEXT:
									case PLAYLIST_CLOCK:
									case PLAYLIST_TEMP:
										playlist[j].data = (u8*)m_calloc(playlist[j].header.length,sizeof(u8));
										memcpy(playlist[j].data,&pageData[index],playlist[j].header.length);
										index += playlist[j].header.length;
										break;
									default:
										pageTemp->popup[i].Info.header.itemCount = j;
										result = F_ERROR;
										break;
								}
								if(result != F_SUCCESS)
									break;
							}							
						}
						break;			
					default:
						result = F_ERROR;
						break;
				}
			}
			if(result != F_SUCCESS)
			{
				pageTemp->header.itemCount = i;
				break;
			}
		}
	}
	if(result == F_SUCCESS)
	{
		result = savePage(pageTemp);
	}
	if(pageTemp != NULL)
		freePageResource(pageTemp);

	return result;
}

//============================================
F_RETURN updatePageHeader(u16 pageID,u8* pageHeader)
{
	PAGE* pageTemp = NULL;
	char name[20] = "";		
	FRESULT rc;
	F_RETURN result = F_SUCCESS;
	
	if(pageTemp != NULL)
		freePageResource(pageTemp);
	pageTemp = loadPage(pageID);
	if(pageTemp != NULL)
	{
		memcpy(&pageTemp[0],&pageHeader[0],PAGE_HEADER_LENGTH);
		// Delete current page
		convert_name(pageID,&name[0],FILE_PAGE);
		rc = f_unlink(&name[0]);
		if(rc == FR_OK)
		{
			// Save new page
			savePage(pageTemp);
		}
		else
			result = F_ERROR;
		freePageResource(pageTemp);
	}
	else
		result = F_ERROR;
				
	return result;
}

//============================================
F_RETURN updatePage(u16 pageID,u8* pageData)
{
	PAGE pageTemp;
	F_RETURN result = F_SUCCESS;

	memcpy(&pageTemp,&pageData[0],PAGE_HEADER_LENGTH);
	if(pageID != pageTemp.header.ID)
		result = F_ERROR;
	else
	{
		result = createPage(pageData);
	}
					
	return result;
}

//============================================
F_RETURN rmPlaylistItem(PAGE* page,u8 popupID,u8 itemID)
{
	POPUP popupTemp;
	u16 i = 0,j = 0,k = 0;
	PLAYLIST_ITEM* playlist;
	PLAYLIST_ITEM* playlist2;
	F_RETURN result = F_SUCCESS;

	for(i = 0;i<page->header.itemCount;i++)
	{
		if(page->popup[i].Info.header.ID == popupID)
		{
			// Copy popup data to temp buffer	
			memcpy(&popupTemp,&page->popup[i],POPUP_HEADER_LENGTH);
			popupTemp.Info.data = (PLAYLIST_ITEM*)m_calloc(popupTemp.Info.header.itemCount,sizeof(PLAYLIST_ITEM));
			playlist2 = (PLAYLIST_ITEM*)popupTemp.Info.data;
			playlist = (PLAYLIST_ITEM*)page->popup[i].Info.data;
			for(j = 0;j<popupTemp.Info.header.itemCount;j++)
			{
				memcpy(&playlist2[j].header,&playlist[j].header,sizeof(PLAYLIST_HEADER));
				playlist2[j].data = (u8*)m_calloc(playlist2[j].header.length,sizeof(u8));
				memcpy(playlist2[j].data,playlist[j].data,playlist[j].header.length);
				free(playlist[j].data);
			}			
			free(playlist);
			// Remove playlist item
			page->popup[i].Info.header.itemCount --;
			if(page->popup[i].Info.header.itemCount>0)
			{
				page->popup[i].Info.data = (PLAYLIST_ITEM*)m_calloc(page->popup[i].Info.header.itemCount,sizeof(PLAYLIST_ITEM));
				playlist = (PLAYLIST_ITEM*)page->popup[i].Info.data;
				k = 0;
				for(j = 0;j<popupTemp.Info.header.itemCount;j++)
				{
					if(playlist2[j].header.playOrder != itemID)
					{
						memcpy(&playlist[k].header,&playlist2[j].header,sizeof(PLAYLIST_HEADER));
						playlist[k].data = (u8*)m_calloc(playlist[k].header.length,sizeof(u8));
						memcpy(playlist[k].data,playlist2[j].data,playlist[k].header.length);
						k++;
					}
				}
			}
			// Free temp buffer
			for(j = 0;j<popupTemp.Info.header.itemCount;j++)
			{
				free(playlist2[j].data);
			}
			free(playlist2);
		}
	}

	return result;
}

//============================================
void convert_name(u16 code,char *buff,FILE_TYPE type)
{
	static char s[33];
	char *ptr;
	u8 count = 0;
	u8 index = 0;
	
	ptr=&s[sizeof(s)-1];
	*ptr='\0';
	do
	{
		*--ptr="0123456789ABCDEF"[code%10];
		code/=10;
		count ++;
	}while(code!=0);

	switch(type)
	{
		case FILE_IMAGE:
			index = sizeof(imgDir);
			memcpy(&buff[0],imgDir,index);
			buff[index] = '/';
			index++;
			memcpy(&buff[index],&ptr[0],count);
			index += count;
			memcpy(&buff[index],".bin",4);
			break;
		case FILE_PAGE:
			index = sizeof(pageDir);
			memcpy(&buff[0],pageDir,index);
			buff[index] = '/';
			index++;
			memcpy(&buff[index],&ptr[0],count);
			index += count;
			memcpy(&buff[index],".dat",4);
			break;
		case FILE_FONT:
			index = sizeof(fontDir);
			memcpy(&buff[0],fontDir,index);
			buff[index] = '/';
			index++;
			memcpy(&buff[index],&ptr[0],count);
			index += count;
			memcpy(&buff[index],".bin",5);
			break;
		default:
			break;
	}
}

//============================================
F_RETURN verifyPopupSize(POPUP p)
{
	F_RETURN result = F_SUCCESS;
	u32 x_temp = 0,y_temp = 0;

	x_temp = (p.Info.header.x + p.Info.header.width);
	y_temp = (p.Info.header.y + p.Info.header.height);
	if((x_temp > hwConfig.ledWidth)
		|| (y_temp > hwConfig.ledHeight))
		result = F_SIZE_ERR;
	else
		result = F_SUCCESS;

	return result;
}

//============================================
F_RETURN verifyPopupHeader(POPUP p)
{
	F_RETURN result = F_SUCCESS;

	result = verifyPopupSize(p);	
	if(result == F_SUCCESS)
	{
		if(p.Info.header.dataType<=UNKNOW || p.Info.header.dataType>PLAYLIST_TEMP)
			result = F_ERROR;
	}

	return result;
}


