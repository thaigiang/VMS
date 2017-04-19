#ifndef __PAGE_MNG__
#define	__PAGE_MNG__

#define PAGE_ROM_ADDRESS 	0x08060400

#define IMG1_ADDR		0x08061C00
#define IMG2_ADDR		0x08063400
#define IMG3_ADDR		0x08064C00
#define IMG4_ADDR		0x08066400
#define IMG5_ADDR		0x08067C00
#define IMG6_ADDR		0x08069400

#define PAGE_HEADER_LENGTH	(sizeof(PAGE_HEADER))
#define POPUP_HEADER_LENGTH	(sizeof(POPUP_HEADER) - sizeof(bool))

//===========================================
void convert_name(u16 code,char *buff,FILE_TYPE type);
PAGE* loadPage(u16 pageID);
F_RETURN savePage(PAGE* page);
void freePageResource(PAGE* page);
void freePopup(POPUP* popup);
F_RETURN createPopup(u16 pageID,u16 popupID,u8* popupData);
F_RETURN updatePopup(u16 pageID,u16 popupID,u8* popupData);
F_RETURN deletePopup(u16 pageID,u16 popupID);
F_RETURN createPage(u8* pageData);
F_RETURN updatePage(u16 pageID,u8* pageData);
F_RETURN updatePageHeader(u16 pageID,u8* pageHeader);
F_RETURN rmPlaylistItem(PAGE* page,u8 popupID,u8 itemID);
F_RETURN verifyPopupSize(POPUP p);
F_RETURN verifyPopupHeader(POPUP p);
bool checkImgExist(u16 imgID);
#endif

