// based on https://www.codeguru.com/cpp/i-n/ieprogram/article.php/c4379/Display-a-Web-Page-in-a-Plain-C-Win32-Application.htm

#include <windows.h>
#include <exdisp.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <crtdbg.h>
#include <shlwapi.h>
#include <wincred.h>
#include <stdio.h>

#include "../htmlwindow.h"
#include "resource.h"


static unsigned int WindowCount = 0;
static const TCHAR *ClassName = "edwork Window";
static const SAFEARRAYBOUND ArrayBound = {1, 0};
static ui_trigger_event callback_event;
static ui_tray_event event_tray_event;
static NOTIFYICONDATAW tray_icon; 
static int gui_lock = 0;
static ui_event ui_callbacks[UI_EVENTS];
static void *ui_data[UI_EVENTS];
static ui_idle_event scheduled_event[UI_SCHEDULER_SIZE];
static void *scheduled_data[UI_SCHEDULER_SIZE];
static int schedule_count = 0;

HRESULT STDMETHODCALLTYPE Storage_QueryInterface(IStorage FAR* This, REFIID riid, LPVOID FAR* ppvObj);
ULONG STDMETHODCALLTYPE Storage_AddRef(IStorage FAR* This);
ULONG STDMETHODCALLTYPE Storage_Release(IStorage FAR* This);
HRESULT STDMETHODCALLTYPE Storage_CreateStream(IStorage FAR* This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm);
HRESULT STDMETHODCALLTYPE Storage_OpenStream(IStorage FAR* This, const WCHAR * pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm);
HRESULT STDMETHODCALLTYPE Storage_CreateStorage(IStorage FAR* This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg);
HRESULT STDMETHODCALLTYPE Storage_OpenStorage(IStorage FAR* This, const WCHAR * pwcsName, IStorage * pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg);
HRESULT STDMETHODCALLTYPE Storage_CopyTo(IStorage FAR* This, DWORD ciidExclude, IID const *rgiidExclude, SNB snbExclude,IStorage *pstgDest);
HRESULT STDMETHODCALLTYPE Storage_MoveElementTo(IStorage FAR* This, const OLECHAR *pwcsName,IStorage * pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags);
HRESULT STDMETHODCALLTYPE Storage_Commit(IStorage FAR* This, DWORD grfCommitFlags);
HRESULT STDMETHODCALLTYPE Storage_Revert(IStorage FAR* This);
HRESULT STDMETHODCALLTYPE Storage_EnumElements(IStorage FAR* This, DWORD reserved1, void * reserved2, DWORD reserved3, IEnumSTATSTG ** ppenum);
HRESULT STDMETHODCALLTYPE Storage_DestroyElement(IStorage FAR* This, const OLECHAR *pwcsName);
HRESULT STDMETHODCALLTYPE Storage_RenameElement(IStorage FAR* This, const WCHAR *pwcsOldName, const WCHAR *pwcsNewName);
HRESULT STDMETHODCALLTYPE Storage_SetElementTimes(IStorage FAR* This, const WCHAR *pwcsName, FILETIME const *pctime, FILETIME const *patime, FILETIME const *pmtime);
HRESULT STDMETHODCALLTYPE Storage_SetClass(IStorage FAR* This, REFCLSID clsid);
HRESULT STDMETHODCALLTYPE Storage_SetStateBits(IStorage FAR* This, DWORD grfStateBits, DWORD grfMask);
HRESULT STDMETHODCALLTYPE Storage_Stat(IStorage FAR* This, STATSTG * pstatstg, DWORD grfStatFlag);

static IStorageVtbl MyIStorageTable = {
    Storage_QueryInterface,
    Storage_AddRef,
    Storage_Release,
    Storage_CreateStream,
    Storage_OpenStream,
    Storage_CreateStorage,
    Storage_OpenStorage,
    Storage_CopyTo,
    Storage_MoveElementTo,
    Storage_Commit,
    Storage_Revert,
    Storage_EnumElements,
    Storage_DestroyElement,
    Storage_RenameElement,
    Storage_SetElementTimes,
    Storage_SetClass,
    Storage_SetStateBits,
    Storage_Stat
};

static IStorage MyIStorage = { &MyIStorageTable };

HRESULT STDMETHODCALLTYPE Frame_QueryInterface(IOleInPlaceFrame FAR* This, REFIID riid, LPVOID FAR* ppvObj);
ULONG STDMETHODCALLTYPE Frame_AddRef(IOleInPlaceFrame FAR* This);
ULONG STDMETHODCALLTYPE Frame_Release(IOleInPlaceFrame FAR* This);
HRESULT STDMETHODCALLTYPE Frame_GetWindow(IOleInPlaceFrame FAR* This, HWND FAR* lphwnd);
HRESULT STDMETHODCALLTYPE Frame_ContextSensitiveHelp(IOleInPlaceFrame FAR* This, BOOL fEnterMode);
HRESULT STDMETHODCALLTYPE Frame_GetBorder(IOleInPlaceFrame FAR* This, LPRECT lprectBorder);
HRESULT STDMETHODCALLTYPE Frame_RequestBorderSpace(IOleInPlaceFrame FAR* This, LPCBORDERWIDTHS pborderwidths);
HRESULT STDMETHODCALLTYPE Frame_SetBorderSpace(IOleInPlaceFrame FAR* This, LPCBORDERWIDTHS pborderwidths);
HRESULT STDMETHODCALLTYPE Frame_SetActiveObject(IOleInPlaceFrame FAR* This, IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);
HRESULT STDMETHODCALLTYPE Frame_InsertMenus(IOleInPlaceFrame FAR* This, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
HRESULT STDMETHODCALLTYPE Frame_SetMenu(IOleInPlaceFrame FAR* This, HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
HRESULT STDMETHODCALLTYPE Frame_RemoveMenus(IOleInPlaceFrame FAR* This, HMENU hmenuShared);
HRESULT STDMETHODCALLTYPE Frame_SetStatusText(IOleInPlaceFrame FAR* This, LPCOLESTR pszStatusText);
HRESULT STDMETHODCALLTYPE Frame_EnableModeless(IOleInPlaceFrame FAR* This, BOOL fEnable);
HRESULT STDMETHODCALLTYPE Frame_TranslateAccelerator(IOleInPlaceFrame FAR* This, LPMSG lpmsg, WORD wID);

static IOleInPlaceFrameVtbl MyIOleInPlaceFrameTable = {
    Frame_QueryInterface,
    Frame_AddRef,
    Frame_Release,
    Frame_GetWindow,
    Frame_ContextSensitiveHelp,
    Frame_GetBorder,
    Frame_RequestBorderSpace,
    Frame_SetBorderSpace,
    Frame_SetActiveObject,
    Frame_InsertMenus,
    Frame_SetMenu,
    Frame_RemoveMenus,
    Frame_SetStatusText,
    Frame_EnableModeless,
    Frame_TranslateAccelerator
};

typedef struct _IOleInPlaceFrameEx {
    IOleInPlaceFrame    frame;        // The IOleInPlaceFrame must be first!
    HWND                window;
} IOleInPlaceFrameEx;


HRESULT STDMETHODCALLTYPE Site_QueryInterface(IOleClientSite FAR* This, REFIID riid, void ** ppvObject);
ULONG STDMETHODCALLTYPE Site_AddRef(IOleClientSite FAR* This);
ULONG STDMETHODCALLTYPE Site_Release(IOleClientSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_SaveObject(IOleClientSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_GetMoniker(IOleClientSite FAR* This, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk);
HRESULT STDMETHODCALLTYPE Site_GetContainer(IOleClientSite FAR* This, LPOLECONTAINER FAR* ppContainer);
HRESULT STDMETHODCALLTYPE Site_ShowObject(IOleClientSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_OnShowWindow(IOleClientSite FAR* This, BOOL fShow);
HRESULT STDMETHODCALLTYPE Site_RequestNewObjectLayout(IOleClientSite FAR* This);

static IOleClientSiteVtbl MyIOleClientSiteTable = {
    Site_QueryInterface,
    Site_AddRef,
    Site_Release,
    Site_SaveObject,
    Site_GetMoniker,
    Site_GetContainer,
    Site_ShowObject,
    Site_OnShowWindow,
    Site_RequestNewObjectLayout
};

HRESULT STDMETHODCALLTYPE Site_GetWindow(IOleInPlaceSite FAR* This, HWND FAR* lphwnd);
HRESULT STDMETHODCALLTYPE Site_ContextSensitiveHelp(IOleInPlaceSite FAR* This, BOOL fEnterMode);
HRESULT STDMETHODCALLTYPE Site_CanInPlaceActivate(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_OnInPlaceActivate(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_OnUIActivate(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_GetWindowContext(IOleInPlaceSite FAR* This, LPOLEINPLACEFRAME FAR* lplpFrame,LPOLEINPLACEUIWINDOW FAR* lplpDoc,LPRECT lprcPosRect,LPRECT lprcClipRect,LPOLEINPLACEFRAMEINFO lpFrameInfo);
HRESULT STDMETHODCALLTYPE Site_Scroll(IOleInPlaceSite FAR* This, SIZE scrollExtent);
HRESULT STDMETHODCALLTYPE Site_OnUIDeactivate(IOleInPlaceSite FAR* This, BOOL fUndoable);
HRESULT STDMETHODCALLTYPE Site_OnInPlaceDeactivate(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_DiscardUndoState(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_DeactivateAndUndo(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_OnPosRectChange(IOleInPlaceSite FAR* This, LPCRECT lprcPosRect);

static IOleInPlaceSiteVtbl MyIOleInPlaceSiteTable = {
    (HRESULT (*)(IOleInPlaceSite *, const IID * const, void **))Site_QueryInterface,
    (ULONG (*)(IOleInPlaceSite *))Site_AddRef,
    (ULONG (*)(IOleInPlaceSite *))Site_Release,
    Site_GetWindow,
    Site_ContextSensitiveHelp,
    Site_CanInPlaceActivate,
    Site_OnInPlaceActivate,
    Site_OnUIActivate,
    Site_GetWindowContext,
    Site_Scroll,
    Site_OnUIDeactivate,
    Site_OnInPlaceDeactivate,
    Site_DiscardUndoState,
    Site_DeactivateAndUndo,
    Site_OnPosRectChange
};

HRESULT STDMETHODCALLTYPE UI_QueryInterface(IDocHostUIHandler FAR* This, REFIID riid, void ** ppvObject);
ULONG STDMETHODCALLTYPE UI_AddRef(IDocHostUIHandler FAR* This);
ULONG STDMETHODCALLTYPE UI_Release(IDocHostUIHandler FAR* This);
HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler FAR* This, DWORD dwID, POINT __RPC_FAR *ppt, IUnknown __RPC_FAR *pcmdtReserved, IDispatch __RPC_FAR *pdispReserved);
HRESULT STDMETHODCALLTYPE UI_GetHostInfo(IDocHostUIHandler FAR* This, DOCHOSTUIINFO __RPC_FAR *pInfo);
HRESULT STDMETHODCALLTYPE UI_ShowUI(IDocHostUIHandler FAR* This, DWORD dwID, IOleInPlaceActiveObject __RPC_FAR *pActiveObject, IOleCommandTarget __RPC_FAR *pCommandTarget, IOleInPlaceFrame __RPC_FAR *pFrame, IOleInPlaceUIWindow __RPC_FAR *pDoc);
HRESULT STDMETHODCALLTYPE UI_HideUI(IDocHostUIHandler FAR* This);
HRESULT STDMETHODCALLTYPE UI_UpdateUI(IDocHostUIHandler FAR* This);
HRESULT STDMETHODCALLTYPE UI_EnableModeless(IDocHostUIHandler FAR* This, BOOL fEnable);
HRESULT STDMETHODCALLTYPE UI_OnDocWindowActivate(IDocHostUIHandler FAR* This, BOOL fActivate);
HRESULT STDMETHODCALLTYPE UI_OnFrameWindowActivate(IDocHostUIHandler FAR* This, BOOL fActivate);
HRESULT STDMETHODCALLTYPE UI_ResizeBorder(IDocHostUIHandler FAR* This, LPCRECT prcBorder, IOleInPlaceUIWindow __RPC_FAR *pUIWindow, BOOL fRameWindow);
HRESULT STDMETHODCALLTYPE UI_TranslateAccelerator(IDocHostUIHandler FAR* This, LPMSG lpMsg, const GUID __RPC_FAR *pguidCmdGroup, DWORD nCmdID);
HRESULT STDMETHODCALLTYPE UI_GetOptionKeyPath(IDocHostUIHandler FAR* This, LPOLESTR __RPC_FAR *pchKey, DWORD dw);
HRESULT STDMETHODCALLTYPE UI_GetDropTarget(IDocHostUIHandler FAR* This, IDropTarget __RPC_FAR *pDropTarget, IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget);
HRESULT STDMETHODCALLTYPE UI_GetExternal(IDocHostUIHandler FAR* This, IDispatch __RPC_FAR *__RPC_FAR *ppDispatch);
HRESULT STDMETHODCALLTYPE UI_TranslateUrl(IDocHostUIHandler FAR* This, DWORD dwTranslate, OLECHAR __RPC_FAR *pchURLIn, OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut);
HRESULT STDMETHODCALLTYPE UI_FilterDataObject(IDocHostUIHandler FAR* This, IDataObject __RPC_FAR *pDO, IDataObject __RPC_FAR *__RPC_FAR *ppDORet);

IDocHostUIHandlerVtbl MyIDocHostUIHandlerTable = {
    UI_QueryInterface,
    UI_AddRef,
    UI_Release,
    UI_ShowContextMenu,
    UI_GetHostInfo,
    UI_ShowUI,
    UI_HideUI,
    UI_UpdateUI,
    UI_EnableModeless,
    UI_OnDocWindowActivate,
    UI_OnFrameWindowActivate,
    UI_ResizeBorder,
    UI_TranslateAccelerator,
    UI_GetOptionKeyPath,
    UI_GetDropTarget,
    UI_GetExternal,
    UI_TranslateUrl,
    UI_FilterDataObject
};

HRESULT STDMETHODCALLTYPE Dispatch_QueryInterface(IDispatch *, REFIID riid, void **);
ULONG STDMETHODCALLTYPE Dispatch_AddRef(IDispatch *);
ULONG STDMETHODCALLTYPE Dispatch_Release(IDispatch *);
HRESULT STDMETHODCALLTYPE Dispatch_GetTypeInfoCount(IDispatch *, unsigned int *);
HRESULT STDMETHODCALLTYPE Dispatch_GetTypeInfo(IDispatch *, unsigned int, LCID, ITypeInfo **);
HRESULT STDMETHODCALLTYPE Dispatch_GetIDsOfNames(IDispatch *, REFIID, OLECHAR **, unsigned int, LCID, DISPID *);
HRESULT STDMETHODCALLTYPE Dispatch_Invoke(IDispatch *, DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, unsigned int *);

IDispatchVtbl MyIDispatchVtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

typedef struct __IOleInPlaceSiteEx {
    IOleInPlaceSite        inplace;        // My IOleInPlaceSite object. Must be first.
    IOleInPlaceFrameEx    *frame;
} _IOleInPlaceSiteEx;

typedef struct __IDispatchEx {
    IDispatch   dispatch; // The mandatory IDispatch.
    HWND        hwnd;
    DWORD       refCount;
} _IDispatchEx;

typedef struct {
    IDocHostUIHandler        ui;            // My IDocHostUIHandler object. Must be first.
    _IDispatchEx            idispatch;
} _IDocHostUIHandlerEx;

typedef struct __IOleClientSiteEx {
    IOleClientSite        client;            // My IOleClientSite object. Must be first.
    _IOleInPlaceSiteEx    inplace;        // My IOleInPlaceSite object.
    _IDocHostUIHandlerEx ui;

    ///////////////////////////////////////////////////
    // Here you add any variables that you need
    // to access in your IOleClientSite functions.
    ///////////////////////////////////////////////////

} _IOleClientSiteEx;

#define NOTIMPLEMENTED return E_NOTIMPL

HRESULT STDMETHODCALLTYPE Storage_QueryInterface(IStorage FAR* This, REFIID riid, LPVOID FAR* ppvObj) {
    NOTIMPLEMENTED;
}

ULONG STDMETHODCALLTYPE Storage_AddRef(IStorage FAR* This) {
    return 1;
}

ULONG STDMETHODCALLTYPE Storage_Release(IStorage FAR* This) {
    return 1;
}

HRESULT STDMETHODCALLTYPE Storage_CreateStream(IStorage FAR* This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_OpenStream(IStorage FAR* This, const WCHAR * pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_CreateStorage(IStorage FAR* This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_OpenStorage(IStorage FAR* This, const WCHAR * pwcsName, IStorage * pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_CopyTo(IStorage FAR* This, DWORD ciidExclude, IID const *rgiidExclude, SNB snbExclude,IStorage *pstgDest) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_MoveElementTo(IStorage FAR* This, const OLECHAR *pwcsName,IStorage * pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_Commit(IStorage FAR* This, DWORD grfCommitFlags) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_Revert(IStorage FAR* This) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_EnumElements(IStorage FAR* This, DWORD reserved1, void * reserved2, DWORD reserved3, IEnumSTATSTG ** ppenum) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_DestroyElement(IStorage FAR* This, const OLECHAR *pwcsName) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_RenameElement(IStorage FAR* This, const WCHAR *pwcsOldName, const WCHAR *pwcsNewName) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_SetElementTimes(IStorage FAR* This, const WCHAR *pwcsName, FILETIME const *pctime, FILETIME const *patime, FILETIME const *pmtime) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_SetClass(IStorage FAR* This, REFCLSID clsid) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Storage_SetStateBits(IStorage FAR* This, DWORD grfStateBits, DWORD grfMask) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_Stat(IStorage FAR* This, STATSTG * pstatstg, DWORD grfStatFlag) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_QueryInterface(IOleClientSite FAR* This, REFIID riid, void ** ppvObject) {
    if (!memcmp(riid, &IID_IUnknown, sizeof(GUID)) || !memcmp(riid, &IID_IOleClientSite, sizeof(GUID)))
        *ppvObject = &((_IOleClientSiteEx *)This)->client;
    else
    if (!memcmp(riid, &IID_IOleInPlaceSite, sizeof(GUID)))
        *ppvObject = &((_IOleClientSiteEx *)This)->inplace;
    else
    if (!memcmp(riid, &IID_IDocHostUIHandler, sizeof(GUID)))
        *ppvObject = &((_IOleClientSiteEx *)This)->ui;
    else {
        *ppvObject = 0;
        return(E_NOINTERFACE);
    }

    return S_OK;
}

ULONG STDMETHODCALLTYPE Site_AddRef(IOleClientSite FAR* This) {
    return 1;
}

ULONG STDMETHODCALLTYPE Site_Release(IOleClientSite FAR* This) {
    return 1;
}

HRESULT STDMETHODCALLTYPE Site_SaveObject(IOleClientSite FAR* This) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_GetMoniker(IOleClientSite FAR* This, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_GetContainer(IOleClientSite FAR* This, LPOLECONTAINER FAR* ppContainer)
{
    // Tell the browser that we are a simple object and don't support a container
    *ppContainer = 0;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE Site_ShowObject(IOleClientSite FAR* This) {
    return NOERROR;
}

HRESULT STDMETHODCALLTYPE Site_OnShowWindow(IOleClientSite FAR* This, BOOL fShow)
{
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_RequestNewObjectLayout(IOleClientSite FAR* This) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_GetWindow(IOleInPlaceSite FAR* This, HWND FAR* lphwnd) {
    *lphwnd = ((_IOleInPlaceSiteEx FAR*)This)->frame->window;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Site_ContextSensitiveHelp(IOleInPlaceSite FAR* This, BOOL fEnterMode) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_CanInPlaceActivate(IOleInPlaceSite FAR* This) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Site_OnInPlaceActivate(IOleInPlaceSite FAR* This) {
    // Tell the browser we did it ok
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Site_OnUIActivate(IOleInPlaceSite FAR* This) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Site_GetWindowContext(IOleInPlaceSite FAR* This, LPOLEINPLACEFRAME FAR* lplpFrame, LPOLEINPLACEUIWINDOW FAR* lplpDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo) {
    *lplpFrame = (LPOLEINPLACEFRAME)((_IOleInPlaceSiteEx FAR*)This)->frame;
    *lplpDoc = 0;

    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = ((IOleInPlaceFrameEx FAR*)*lplpFrame)->window;
    lpFrameInfo->haccel = 0;
    lpFrameInfo->cAccelEntries = 0;

    GetClientRect(lpFrameInfo->hwndFrame, lprcPosRect);
    GetClientRect(lpFrameInfo->hwndFrame, lprcClipRect);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE Site_Scroll(IOleInPlaceSite FAR* This, SIZE scrollExtent) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_OnUIDeactivate(IOleInPlaceSite FAR* This, BOOL fUndoable) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Site_OnInPlaceDeactivate(IOleInPlaceSite FAR* This) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Site_DiscardUndoState(IOleInPlaceSite FAR* This) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_DeactivateAndUndo(IOleInPlaceSite FAR* This) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_OnPosRectChange(IOleInPlaceSite FAR* This, LPCRECT lprcPosRect) {
    IOleObject            *browserObject;
    IOleInPlaceObject    *inplace;

    HWND hwnd;
    This->lpVtbl->GetWindow(This, &hwnd);

    browserObject = *((IOleObject **)GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IOleInPlaceObject, (void**)&inplace)) 
        inplace->lpVtbl->SetObjectRects(inplace, lprcPosRect, lprcPosRect);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE Frame_QueryInterface(IOleInPlaceFrame FAR* This, REFIID riid, LPVOID FAR* ppvObj) {
    NOTIMPLEMENTED;
}

ULONG STDMETHODCALLTYPE Frame_AddRef(IOleInPlaceFrame FAR* This) {
    return 1;
}

ULONG STDMETHODCALLTYPE Frame_Release(IOleInPlaceFrame FAR* This) {
    return 1;
}

HRESULT STDMETHODCALLTYPE Frame_GetWindow(IOleInPlaceFrame FAR* This, HWND FAR* lphwnd) {
    *lphwnd = ((IOleInPlaceFrameEx FAR*)This)->window;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Frame_ContextSensitiveHelp(IOleInPlaceFrame FAR* This, BOOL fEnterMode) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_GetBorder(IOleInPlaceFrame FAR* This, LPRECT lprectBorder) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_RequestBorderSpace(IOleInPlaceFrame FAR* This, LPCBORDERWIDTHS pborderwidths) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetBorderSpace(IOleInPlaceFrame FAR* This, LPCBORDERWIDTHS pborderwidths) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetActiveObject(IOleInPlaceFrame FAR* This, IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Frame_InsertMenus(IOleInPlaceFrame FAR* This, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetMenu(IOleInPlaceFrame FAR* This, HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Frame_RemoveMenus(IOleInPlaceFrame FAR* This, HMENU hmenuShared) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetStatusText(IOleInPlaceFrame FAR* This, LPCOLESTR pszStatusText) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Frame_EnableModeless(IOleInPlaceFrame FAR* This, BOOL fEnable) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Frame_TranslateAccelerator(IOleInPlaceFrame FAR* This, LPMSG lpmsg, WORD wID) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE UI_QueryInterface(IDocHostUIHandler FAR* This, REFIID riid, LPVOID FAR* ppvObj) {
    return(Site_QueryInterface((IOleClientSite *)((char *)This - sizeof(IOleClientSite) - sizeof(_IOleInPlaceSiteEx)), riid, ppvObj));
}

ULONG STDMETHODCALLTYPE UI_AddRef(IDocHostUIHandler FAR* This) {
    return 1;
}

ULONG STDMETHODCALLTYPE UI_Release(IDocHostUIHandler FAR* This) {
    return 1;
}

// Called when the browser object is about to display its context menu.
HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler FAR* This, DWORD dwID, POINT __RPC_FAR *ppt, IUnknown __RPC_FAR *pcmdtReserved, IDispatch __RPC_FAR *pdispReserved) {
    return S_OK;
}

// Called at initialization of the browser object UI. We can set various features of the browser object here.
HRESULT STDMETHODCALLTYPE UI_GetHostInfo(IDocHostUIHandler FAR* This, DOCHOSTUIINFO __RPC_FAR *pInfo) {
    pInfo->cbSize = sizeof(DOCHOSTUIINFO);
    pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER;
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    return S_OK;
}

// Called when the browser object shows its UI.
HRESULT STDMETHODCALLTYPE UI_ShowUI(IDocHostUIHandler FAR* This, DWORD dwID, IOleInPlaceActiveObject __RPC_FAR *pActiveObject, IOleCommandTarget __RPC_FAR *pCommandTarget, IOleInPlaceFrame __RPC_FAR *pFrame, IOleInPlaceUIWindow __RPC_FAR *pDoc) {
    return S_OK;
}

// Called when browser object hides its UI. This allows us to hide any menus/toolbars we created in ShowUI.
HRESULT STDMETHODCALLTYPE UI_HideUI(IDocHostUIHandler FAR* This) {
    return S_OK;
}

// Called when the browser object wants to notify us that the command state has changed.
HRESULT STDMETHODCALLTYPE UI_UpdateUI(IDocHostUIHandler FAR* This) {
    // We update our UI in our window message loop so we don't do anything here.
    return S_OK;
}

// Called from the browser object's IOleInPlaceActiveObject object's EnableModeless() function.
HRESULT STDMETHODCALLTYPE UI_EnableModeless(IDocHostUIHandler FAR* This, BOOL fEnable) {
    return S_OK;
}

// Called from the browser object's IOleInPlaceActiveObject object's OnDocWindowActivate() function.
HRESULT STDMETHODCALLTYPE UI_OnDocWindowActivate(IDocHostUIHandler FAR* This, BOOL fActivate) {
    return S_OK;
}

// Called from the browser object's IOleInPlaceActiveObject object's OnFrameWindowActivate() function.
HRESULT STDMETHODCALLTYPE UI_OnFrameWindowActivate(IDocHostUIHandler FAR* This, BOOL fActivate) {
    return S_OK;
}

// Called from the browser object's IOleInPlaceActiveObject object's ResizeBorder() function.
HRESULT STDMETHODCALLTYPE UI_ResizeBorder(IDocHostUIHandler FAR* This, LPCRECT prcBorder, IOleInPlaceUIWindow __RPC_FAR *pUIWindow, BOOL fRameWindow) {
    return S_OK;
}

// Called from the browser object's TranslateAccelerator routines to translate key strokes to commands.
HRESULT STDMETHODCALLTYPE UI_TranslateAccelerator(IDocHostUIHandler FAR* This, LPMSG lpMsg, const GUID __RPC_FAR *pguidCmdGroup, DWORD nCmdID) {
    NOTIMPLEMENTED;
}

// Called by the browser object to find where the host wishes the browser to get its options in the registry.
HRESULT STDMETHODCALLTYPE UI_GetOptionKeyPath(IDocHostUIHandler FAR* This, LPOLESTR __RPC_FAR *pchKey, DWORD dw) {
    // Let the browser use its default registry settings.
    NOTIMPLEMENTED;
}

// Called by the browser object when it is used as a drop target
HRESULT STDMETHODCALLTYPE UI_GetDropTarget(IDocHostUIHandler FAR* This, IDropTarget __RPC_FAR *pDropTarget, IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget) {
    NOTIMPLEMENTED;
}

// Called by the browser when it wants a pointer to our IDispatch object
HRESULT STDMETHODCALLTYPE UI_GetExternal(IDocHostUIHandler FAR* This, IDispatch __RPC_FAR *__RPC_FAR *ppDispatch) {
    *ppDispatch = (IDispatch *)&((_IDocHostUIHandlerEx FAR *)This)->idispatch;
    if (*ppDispatch)
        return S_OK;

    *ppDispatch = 0;
    NOTIMPLEMENTED;
}

// Called by the browser object to give us an opportunity to modify the URL to be loaded.
HRESULT STDMETHODCALLTYPE UI_TranslateUrl(IDocHostUIHandler FAR* This, DWORD dwTranslate, OLECHAR __RPC_FAR *pchURLIn, OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut) {
    *ppchURLOut = 0;
    NOTIMPLEMENTED;
}

// Called by the browser when it does cut/paste to the clipboard
HRESULT STDMETHODCALLTYPE UI_FilterDataObject(IDocHostUIHandler FAR* This, IDataObject __RPC_FAR *pDO, IDataObject __RPC_FAR *__RPC_FAR *ppDORet) {
    *ppDORet = 0;
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Dispatch_QueryInterface(IDispatch * This, REFIID riid, void **ppvObject) {
    *ppvObject = 0;

    if (!memcmp(riid, &IID_IUnknown, sizeof(GUID)) || !memcmp(riid, &IID_IDispatch, sizeof(GUID))) {
        *ppvObject = (void *)This;
        // Increment its usage count. The caller will be expected to call our
        // IDispatch's Release() (ie, Dispatch_Release) when it's done with
        // our IDispatch.
        Dispatch_AddRef(This);

        return(S_OK);
    }

    *ppvObject = 0;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE Dispatch_AddRef(IDispatch *This) {
    return InterlockedIncrement(&((_IDispatchEx *)This)->refCount);
}

ULONG STDMETHODCALLTYPE Dispatch_Release(IDispatch *This) {
    if (InterlockedDecrement( &((_IDispatchEx *)This)->refCount ) == 0)
        return 0;

    return ((_IDispatchEx *)This)->refCount;
}

HRESULT STDMETHODCALLTYPE Dispatch_GetTypeInfoCount(IDispatch *This, unsigned int *pctinfo) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Dispatch_GetTypeInfo(IDispatch *This, unsigned int iTInfo, LCID lcid, ITypeInfo **ppTInfo) {
    NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Dispatch_GetIDsOfNames(IDispatch *This, REFIID riid, OLECHAR ** rgszNames, unsigned int cNames, LCID lcid, DISPID * rgDispId) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Dispatch_Invoke(IDispatch *This, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, unsigned int *puArgErr) {
    IWebBrowser2    *webBrowser2;
    IOleObject        *browserObject;
    IDispatch       *disp;
    IHTMLDocument2  *document;
    // IHTMLEventObj   *htmlEvent;
    // LPCTSTR         eventStr;

    if (!callback_event)
        return S_OK;

    HWND hwnd = (HWND)((_IDispatchEx *)This)->hwnd;
    browserObject = *((IOleObject **)GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if ((browserObject) && (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))) {
        webBrowser2->lpVtbl->get_Document(webBrowser2, &disp);
        if (disp) {
            if (!disp->lpVtbl->QueryInterface(disp, &IID_IHTMLDocument2, (void **)&document)) {
                IHTMLWindow2 *window = NULL;
                document->lpVtbl->get_parentWindow(document, &window);
                if (window) {
                    // if ((!window->lpVtbl->get_event(window, &htmlEvent)) && (htmlEvent)) {
                    // }
                    callback_event(hwnd);
                    window->lpVtbl->Release(window);
                }
                document->lpVtbl->Release(document);
            }
            disp->lpVtbl->Release(disp);
        }
        webBrowser2->lpVtbl->Release(webBrowser2);
    }
    return S_OK;
}

void UnEmbedBrowserObject(HWND hwnd) {
    IOleObject    **browserHandle;
    IOleObject    *browserObject;

    if ((browserHandle = (IOleObject **)GetWindowLongPtr(hwnd, GWLP_USERDATA))) {
        browserObject = *browserHandle;
        browserObject->lpVtbl->Close(browserObject, OLECLOSE_NOSAVE);
        browserObject->lpVtbl->Release(browserObject);
        GlobalFree(browserHandle);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return;
    }
}

long DisplayHTMLStr(HWND hwnd, LPCTSTR string) {    
    IWebBrowser2    *webBrowser2;
    LPDISPATCH        lpDispatch;
    IHTMLDocument2    *htmlDoc2;
    IOleObject        *browserObject;
    SAFEARRAY        *sfArray;
    VARIANT            myURL;
    VARIANT            *pVar;
    BSTR            bstr;

    browserObject = *((IOleObject **)GetWindowLongPtr(hwnd, GWLP_USERDATA));
    bstr = 0;

    char app_path[MAX_PATH];
    if (!GetModuleFileNameA(NULL, app_path, MAX_PATH))
        return -1;

    PathRemoveFileSpecA(app_path);

    int i = 0;
    while (app_path[i]) {
        if (app_path[i] == '\\')
            app_path[i] = '/';
        i ++;
    }
    int len = strlen(string);
    LPSTR string2 = (LPSTR)malloc(len + MAX_PATH + 32);
    snprintf(string2, len + MAX_PATH + 32, "<base href='file:///%s/'/>\n%s", app_path, string);

    if (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2)) {
        VariantInit(&myURL);
        myURL.vt = VT_BSTR;
        myURL.bstrVal = SysAllocString(L"about:blank");
        webBrowser2->lpVtbl->Navigate2(webBrowser2, &myURL, 0, 0, 0, 0);
        VariantClear(&myURL);
        if (!webBrowser2->lpVtbl->get_Document(webBrowser2, &lpDispatch)) {
            if (!lpDispatch->lpVtbl->QueryInterface(lpDispatch, &IID_IHTMLDocument2, (void**)&htmlDoc2)) {
                if ((sfArray = SafeArrayCreate(VT_VARIANT, 1, (SAFEARRAYBOUND *)&ArrayBound))) {
                    if (!SafeArrayAccessData(sfArray, (void**)&pVar)) {
                        pVar->vt = VT_BSTR;
#ifndef UNICODE
                        {
                            wchar_t        *buffer;
                            DWORD        size;

                            size = MultiByteToWideChar(CP_UTF8, 0, string2, -1, 0, 0);
                            if (!(buffer = (wchar_t *)GlobalAlloc(GMEM_FIXED, sizeof(wchar_t) * size)))
                                goto bad;
                            MultiByteToWideChar(CP_UTF8, 0, string2, -1, buffer, size);
                            bstr = SysAllocString(buffer);
                            GlobalFree(buffer);
                        }
#else
                        bstr = SysAllocString(string2);
#endif
                        if ((pVar->bstrVal = bstr)) {
                            htmlDoc2->lpVtbl->write(htmlDoc2, sfArray);
                            // Normally, we'd need to free our BSTR, but SafeArrayDestroy() does it for us
                            // SysFreeString(bstr);
                        }
                    }

                    // Free the array. This also frees the VARIENT that SafeArrayAccessData created for us,
                    // and even frees the BSTR we allocated with SysAllocString
                    SafeArrayDestroy(sfArray);
                }

bad:            htmlDoc2->lpVtbl->Release(htmlDoc2);
            }
            lpDispatch->lpVtbl->Release(lpDispatch);
        }

        webBrowser2->lpVtbl->Release(webBrowser2);
    }
    free(string2);

    if (bstr)
        return 0;

    return -1;
}

long DisplayHTMLPage(HWND hwnd, LPTSTR webPageName) {
    IWebBrowser2    *webBrowser2;
    VARIANT            myURL;
    IOleObject        *browserObject;

    browserObject = *((IOleObject **)GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2)) {
        VariantInit(&myURL);
        myURL.vt = VT_BSTR;

#ifndef UNICODE
        {
        wchar_t        *buffer;
        DWORD        size;

        size = MultiByteToWideChar(CP_UTF8, 0, webPageName, -1, 0, 0);
        if (!(buffer = (wchar_t *)GlobalAlloc(GMEM_FIXED, sizeof(wchar_t) * size)))
            goto badalloc;
        MultiByteToWideChar(CP_UTF8, 0, webPageName, -1, buffer, size);
        myURL.bstrVal = SysAllocString(buffer);
        GlobalFree(buffer);
        }
#else
        myURL.bstrVal = SysAllocString(webPageName);
#endif
        if (!myURL.bstrVal) {
badalloc:    webBrowser2->lpVtbl->Release(webBrowser2);
            return -6;
        }
        webBrowser2->lpVtbl->Navigate2(webBrowser2, &myURL, 0, 0, 0, 0);
        VariantClear(&myURL);
        webBrowser2->lpVtbl->Release(webBrowser2);
        return 0;
    }

    return -5;
}

long EmbedBrowserObject(HWND hwnd) {
    IOleObject            *browserObject;
    IWebBrowser2        *webBrowser2;
    RECT                rect;
    char                *ptr;
    IOleInPlaceFrameEx    *iOleInPlaceFrameEx;
    _IOleClientSiteEx    *_iOleClientSiteEx;

    if (!(ptr = (char *)GlobalAlloc(GMEM_FIXED, sizeof(IOleInPlaceFrameEx) + sizeof(_IOleClientSiteEx) + sizeof(IOleObject *))))
        return -1;
    
    iOleInPlaceFrameEx = (IOleInPlaceFrameEx *)(ptr + sizeof(IOleObject *));
    iOleInPlaceFrameEx->frame.lpVtbl = &MyIOleInPlaceFrameTable;
    iOleInPlaceFrameEx->window = hwnd;
    _iOleClientSiteEx = (_IOleClientSiteEx *)(ptr + sizeof(IOleInPlaceFrameEx) + sizeof(IOleObject *));
    _iOleClientSiteEx->client.lpVtbl = &MyIOleClientSiteTable;
    _iOleClientSiteEx->inplace.inplace.lpVtbl = &MyIOleInPlaceSiteTable;
    _iOleClientSiteEx->inplace.frame = iOleInPlaceFrameEx;
    _iOleClientSiteEx->ui.ui.lpVtbl = &MyIDocHostUIHandlerTable;
    _iOleClientSiteEx->ui.idispatch.dispatch.lpVtbl = &MyIDispatchVtbl;
    _iOleClientSiteEx->ui.idispatch.hwnd = hwnd;
    _iOleClientSiteEx->ui.idispatch.refCount = 0;
    if (!OleCreate(&CLSID_WebBrowser, &IID_IOleObject, OLERENDER_DRAW, 0, (IOleClientSite *)_iOleClientSiteEx, &MyIStorage, (void**)&browserObject)) {
        *((IOleObject **)ptr) = browserObject;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ptr);
        browserObject->lpVtbl->SetHostNames(browserObject, L"edwork settings", 0);
        GetClientRect(hwnd, &rect);
        if ((!OleSetContainedObject((struct IUnknown *)browserObject, TRUE)) &&
            (!browserObject->lpVtbl->DoVerb(browserObject, OLEIVERB_SHOW, NULL, (IOleClientSite *)_iOleClientSiteEx, -1, hwnd, &rect)) &&
            (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))) {
            webBrowser2->lpVtbl->put_Left(webBrowser2, 0);
            webBrowser2->lpVtbl->put_Top(webBrowser2, 0);
            webBrowser2->lpVtbl->put_Width(webBrowser2, rect.right);
            webBrowser2->lpVtbl->put_Height(webBrowser2, rect.bottom);
            webBrowser2->lpVtbl->Release(webBrowser2);
            return 0;
        }

        UnEmbedBrowserObject(hwnd);
        return -3;
    }

    GlobalFree(ptr);
    return -2;
}

void WindowResize(HWND hwnd, int width, int height) {    
    IWebBrowser2    *webBrowser2;
    IOleObject        *browserObject;

    browserObject = *((IOleObject **)GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if ((browserObject) && (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))) {
        webBrowser2->lpVtbl->put_Width(webBrowser2, width);
        webBrowser2->lpVtbl->put_Height(webBrowser2, height);
        webBrowser2->lpVtbl->Release(webBrowser2);
    }
}

LRESULT CALLBACK MenuWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg) {
        case WM_CREATE:
            WindowCount ++;
            return 0;

        case WM_DESTROY:
            if (WindowCount)
                WindowCount --;
            if ((!WindowCount) && (!gui_lock))
                PostQuitMessage(0);
            return 1;

        case WM_APP + 1:
		    switch (lParam)  {
		        case WM_LBUTTONUP:
                case WM_RBUTTONUP:
                  if (event_tray_event)
                      event_tray_event(hwnd);
		          break;

		    }
            return 1;
   }
   return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            if (EmbedBrowserObject(hwnd))
                return -1;
            WindowCount ++;
            if (ui_callbacks[UI_EVENT_WINDOW_CREATE])
                ui_callbacks[UI_EVENT_WINDOW_CREATE](hwnd, ui_data[UI_EVENT_WINDOW_CREATE]);
            return 0;
        case WM_DESTROY:
            if (ui_callbacks[UI_EVENT_WINDOW_CLOSE])
                ui_callbacks[UI_EVENT_WINDOW_CLOSE](hwnd, ui_data[UI_EVENT_WINDOW_CLOSE]);
            UnEmbedBrowserObject(hwnd);
            if (WindowCount)
                WindowCount --;

            if ((!WindowCount) && (!gui_lock))
                PostQuitMessage(0);
            return 1;
        case WM_SIZE:
            WindowResize(hwnd, LOWORD(lParam), HIWORD(lParam));
            return 1;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int ui_app_init(ui_trigger_event event_handler) {
    HKEY key;
    char app_path[MAX_PATH];
    char *app_name;

    callback_event = event_handler;
    if (OleInitialize(NULL) != S_OK) {
        MessageBox(0, "Can't open OLE!", "ERROR", MB_OK);
        return -1;
    }

    if (!GetModuleFileNameA(NULL, app_path, MAX_PATH))
        return 0;

    if (RegOpenKeyA(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION"), &key) != ERROR_SUCCESS)
        return 0;

    app_name = strrchr(app_path, '\\');
    if (app_name)
        app_name ++;
    else
        app_name = app_path;

    DWORD version = 11000;
    RegSetValueExA(key, TEXT(app_name), 0, REG_DWORD, (LPBYTE)&version, sizeof(DWORD));
    RegCloseKey(key);

    return 1;
}

int ui_app_done() {
    OleUninitialize();
    return 1;
}

void ui_app_tray_icon(const char *tooltip, const char *notification_title, const char *notification_text, ui_tray_event event_tray) {
    int exists = tray_icon.uID;

    if (!exists) {
        WNDCLASS wc;
        ZeroMemory(&wc, sizeof(WNDCLASS));
	    wc.lpfnWndProc = MenuWindowProc;			
	    wc.lpszClassName = "EdwordkTrayWindow";			

	    RegisterClass(&wc);

        tray_icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        tray_icon.cbSize = sizeof(tray_icon);
        tray_icon.uCallbackMessage = WM_APP + 1;
        tray_icon.hWnd = CreateWindow("EdwordkTrayWindow", "EdwordkTrayWindow", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
        tray_icon.hIcon = LoadIcon((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1)); 
        tray_icon.uID = 1;
        tray_icon.uFlags |= NIF_INFO;
    }

    if ((notification_title) || (notification_text)) {
        tray_icon.dwInfoFlags = NIIF_INFO;
        tray_icon.uTimeout    = 3000;

        if (notification_title)
            MultiByteToWideChar(CP_UTF8, 0, notification_title, -1, tray_icon.szInfoTitle, 64);
            // strncpy(tray_icon.szInfoTitle, notification_title, 64);

        if (notification_text)
            MultiByteToWideChar(CP_UTF8, 0, notification_text, -1, tray_icon.szInfo, 256);
            // strncpy(tray_icon.szInfo, notification_text, 256);
    }

    if (tooltip)
        MultiByteToWideChar(CP_UTF8, 0, tooltip, -1, tray_icon.szTip, sizeof(tray_icon.szTip) / 2);
        // strncpy(tray_icon.szTip, tooltip, sizeof(tray_icon.szTip));
    else
        tray_icon.szTip[0] = 0;

    if (Shell_NotifyIconW(exists ? NIM_MODIFY : NIM_ADD, &tray_icon))
        event_tray_event = event_tray;
}

void ui_app_tray_remove() {
    int exists = tray_icon.uID;
    if (exists)
        Shell_NotifyIconW(NIM_DELETE, &tray_icon);
}

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime) {
    // nothing
}

void ui_set_event(int eid, ui_event callback, void *event_userdata) {
    if ((eid < 0) || (eid >= UI_EVENTS))
        return;

    ui_callbacks[eid] = callback;
    ui_data[eid] = event_userdata;
}

void ui_app_run_with_notify(ui_idle_event event_idle, void *userdata) {
    MSG msg;
    SetTimer(NULL, 0, 1000, (TIMERPROC)TimerProc);
    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (event_idle)
            event_idle(userdata);
        if (schedule_count > 0) {
            int i;
            for (i = 0; i < UI_SCHEDULER_SIZE; i ++) {
                if (scheduled_event[i]) {
                    scheduled_event[i](scheduled_data[i]);
                    scheduled_event[i] = NULL;
                    scheduled_data[i] = NULL;
                } else
                    break;
            }
            schedule_count = 0;
        }
    }
    KillTimer(NULL, 0);
    ui_app_tray_remove();

    if (ui_callbacks[UI_EVENT_LOOP_EXIT])
        ui_callbacks[UI_EVENT_LOOP_EXIT](NULL, ui_data[UI_EVENT_LOOP_EXIT]);
}

void ui_app_iterate() {
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (schedule_count > 0) {
            int i;
            for (i = 0; i < UI_SCHEDULER_SIZE; i ++) {
                if (scheduled_event[i]) {
                    scheduled_event[i](scheduled_data[i]);
                    scheduled_event[i] = NULL;
                    scheduled_data[i] = NULL;
                } else
                    break;
            }
            schedule_count = 0;
        }
    }
}

void ui_app_run_schedule_once(ui_idle_event scheduled, void *userdata) {
    if (schedule_count >= UI_SCHEDULER_SIZE)
        schedule_count = 0;
    scheduled_event[schedule_count] = scheduled;
    scheduled_data[schedule_count] = userdata;
    schedule_count ++;
}

void ui_app_run() {
    ui_app_run_with_notify(NULL, NULL);
}

void ui_app_quit() {
    PostQuitMessage(0);
}

BSTR MakeBSTR(const char *js) {
    BSTR bstr;
#ifndef UNICODE
    {
        wchar_t        *buffer;
        DWORD        size;

        size = MultiByteToWideChar(CP_UTF8, 0, js, -1, 0, 0);
        if (!(buffer = (wchar_t *)GlobalAlloc(GMEM_FIXED, sizeof(wchar_t) * size)))
            return NULL;
        MultiByteToWideChar(CP_UTF8, 0, js, -1, buffer, size);
        bstr = SysAllocString(buffer);
        GlobalFree(buffer);
    }
#else
    bstr = SysAllocString(js);
#endif
    return bstr;
}

void ui_message(const char *title, const char *body, int level) {
    BSTR wtitle = MakeBSTR(title);
    BSTR wbody = MakeBSTR(body);

    switch (level) {
        case 1:
            MessageBoxW(0, wbody, wtitle, MB_OK | MB_ICONINFORMATION);
            break;
        case 2:
            MessageBoxW(0, wbody, wtitle, MB_OK | MB_ICONWARNING);
            break;
        case 3:
            MessageBoxW(0, wbody, wtitle, MB_OK | MB_ICONEXCLAMATION);
            break;
        default:
            MessageBoxW(0, wbody, wtitle, MB_OK);
    }

    if (wtitle)
        SysFreeString(wtitle);
    if (wbody)
        SysFreeString(wbody);
}

int ui_question(const char *title, const char *body, int level) {
    BSTR wtitle = MakeBSTR(title);
    BSTR wbody = MakeBSTR(body);
    int yes_or_no = 0;

    switch (level) {
        case 1:
            yes_or_no = MessageBoxW(0, wbody, wtitle, MB_YESNO | MB_ICONINFORMATION);
            break;
        case 2:
            yes_or_no = MessageBoxW(0, wbody, wtitle, MB_YESNO | MB_ICONWARNING);
            break;
        case 3:
            yes_or_no = MessageBoxW(0, wbody, wtitle, MB_YESNO | MB_ICONEXCLAMATION);
            break;
        default:
            yes_or_no = MessageBoxW(0, wbody, wtitle, MB_YESNO);
    }

    if (wtitle)
        SysFreeString(wtitle);
    if (wbody)
        SysFreeString(wbody);

    if (yes_or_no == IDYES)
        return 1;

    return 0;
}

int ui_input(const char *title, const char *body, char *val, int val_len, int masked) {
    // unmasked text not supported
    if ((!masked) || (!val) || (!val_len))
        return 0;
    CREDUI_INFO info;
    TCHAR pszName[CREDUI_MAX_USERNAME_LENGTH + 1];
    BOOL fSave;
    DWORD dwErr;
 
    info.cbSize = sizeof(CREDUI_INFO);
    info.hwndParent = NULL;

    info.pszMessageText = body ? body : "";
    info.pszCaptionText = title ? title : "";
    info.hbmBanner = NULL;
    fSave = FALSE;
    SecureZeroMemory(pszName, sizeof(pszName));
    SecureZeroMemory(val, val_len);
    dwErr = CredUIPromptForCredentials(&info, TEXT("edwork"), NULL, 0, pszName, 0, val, val_len - 1, &fSave, CREDUI_FLAGS_KEEP_USERNAME | CREDUI_FLAGS_PASSWORD_ONLY_OK | CREDUI_FLAGS_GENERIC_CREDENTIALS | CREDUI_FLAGS_ALWAYS_SHOW_UI | CREDUI_FLAGS_DO_NOT_PERSIST);

    if (!dwErr)
        return 1;

    return 0;
}

void ui_js(void *wnd, const char *js) {
    IWebBrowser2    *webBrowser2;
    IOleObject        *browserObject;
    IDispatch       *disp;
    IHTMLDocument2  *document;
    BSTR            bstr;
    BSTR            javascript;
    VARIANT         data;

    HWND hwnd = (HWND)wnd;
    browserObject = *((IOleObject **)GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if ((browserObject) && (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))) {
        webBrowser2->lpVtbl->get_Document(webBrowser2, &disp);
        if (disp) {
            if (!disp->lpVtbl->QueryInterface(disp, &IID_IHTMLDocument2, (void **)&document)) {
                IHTMLWindow2 *window = NULL;
                document->lpVtbl->get_parentWindow(document, &window);
                if (window) {
                    bstr = MakeBSTR(js);
                    if (bstr) {
                        javascript = MakeBSTR("JavaScript");
                        VariantInit(&data);
                        window->lpVtbl->execScript(window, bstr, javascript, &data);
                        VariantClear(&data);
                        if (javascript)
                            SysFreeString(javascript);
                        SysFreeString(bstr);
                    }
                    window->lpVtbl->Release(window);
                }
                document->lpVtbl->Release(document);
            }
            disp->lpVtbl->Release(disp);
        }
        webBrowser2->lpVtbl->Release(webBrowser2);
    }
}

int ui_window_count() {
    return WindowCount;
}

void ui_window_close(void *wnd) {
    if (wnd)
        PostMessage((HWND)wnd, WM_CLOSE, 0, 0);
}

void ui_window_maximize(void *wnd) {
    if (wnd)
        ShowWindow((HWND)wnd, SW_MAXIMIZE);
}

void ui_window_minimize(void *wnd) {
    if (wnd)
        ShowWindow((HWND)wnd, SW_MINIMIZE);
}

void ui_window_restore(void *wnd) {
    if (wnd)
        ShowWindow((HWND)wnd, SW_RESTORE);
}

void ui_window_top(void *wnd) {
    if (wnd)
        SetForegroundWindow(wnd);
}

char *ui_call(void *wnd, const char *function, const char *arguments[]) {
    IWebBrowser2    *webBrowser2;
    IOleObject        *browserObject;
    IDispatch       *disp;
    IHTMLDocument2  *document;
    BSTR            bstr;
    VARIANT         data;
    char            *strval = NULL;

    HWND hwnd = (HWND)wnd;
    browserObject = *((IOleObject **)GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if ((browserObject) && (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))) {
        webBrowser2->lpVtbl->get_Document(webBrowser2, &disp);
        if (disp) {
            if (!disp->lpVtbl->QueryInterface(disp, &IID_IHTMLDocument2, (void **)&document)) {
                IDispatch *script = 0;
                document->lpVtbl->get_Script(document, &script);
                if (script) {
                    bstr = MakeBSTR(function);
                    if (bstr) {
                        DISPID dispID;
                        HRESULT hr = script->lpVtbl->GetIDsOfNames(script, &IID_NULL, &bstr, 1, LOCALE_SYSTEM_DEFAULT, &dispID);
                        if (SUCCEEDED(hr)) {
                            VariantInit(&data);

                            VARIANT parameters[32]; 

                            int arg = 0;
                            int i;
                            if (arguments) { 
                                while (arguments[arg]) { 
                                    arg++;
                                    if (arg == 32)
                                        break;
                                }

                                for (i = 0; i < arg; i ++) {
                                    VariantInit(&parameters[i]); 
                                    parameters[i].vt = VT_BSTR; 
                                    parameters[i].bstrVal = MakeBSTR(arguments[arg - i - 1]);
                                }
                            }

                            DISPPARAMS dpNoArgs = {parameters, NULL, arg, 0};

                            hr = script->lpVtbl->Invoke(script, dispID, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dpNoArgs, &data, NULL, NULL);

                            for (i = 0; i < arg; i++)
                                VariantClear(&parameters[i]);

                            if (data.vt == VT_BSTR) {
                                int len = SysStringByteLen(data.bstrVal);
                                strval = (char *)malloc(len + 1);
                                if (strval) {
                                    wcstombs(strval, data.bstrVal, len);
                                    strval[len] = 0;
                                }
                            }
                            VariantClear(&data);
                        }
                        SysFreeString(bstr);
                    }
                    script->lpVtbl->Release(script);
                }
                document->lpVtbl->Release(document);
            }
            disp->lpVtbl->Release(disp);
        }
        webBrowser2->lpVtbl->Release(webBrowser2);
    }
    return strval;
}

void ui_free_string(void *ptr) {
    free(ptr);
}

void ui_window_set_content(void *wnd, const char *body) {
    if (wnd) {
        UnEmbedBrowserObject((HWND)wnd);
        EmbedBrowserObject((HWND)wnd);
        DisplayHTMLStr((HWND)wnd, body);
    }
}

void *ui_window(const char *title, const char *body) {
    WNDCLASSEX wc;
    HWND hwnd;

    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.hInstance = (HINSTANCE)GetModuleHandle(NULL);
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = &ClassName[0];
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hIconSm = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
    RegisterClassEx(&wc);

    if ((hwnd = CreateWindowEx(0, ClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, HWND_DESKTOP, NULL, GetModuleHandle(NULL), 0))) {
        DisplayHTMLStr(hwnd, body);
        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);
        return hwnd;
    }
    return NULL;
}

void ui_lock() { 
    gui_lock ++;
}

void ui_unlock() {
    if (gui_lock) {
        gui_lock --;
        if ((!WindowCount) && (!gui_lock))
            PostQuitMessage(0);
    }
}
