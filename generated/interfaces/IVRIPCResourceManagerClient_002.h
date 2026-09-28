#pragma once
#include "openvr.h"
#include "version.h"
#include "public_vrtypes.h"
#include "vrtypes.h"
#include "vrannotation.h"
// ivripcresourcemanagerclient.h

namespace vr
{
namespace IVRIPCResourceManagerClient_002
{

// -----------------------------------------------------------------------------
// Purpose: Interact with the IPCResourceManager
// -----------------------------------------------------------------------------
class IVRIPCResourceManagerClient
{
public:
	/** Create a new tracked Vulkan Image
	 *
	 * nImageFormat: in VkFormat
	 */
	virtual bool NewSharedVulkanImage( uint32_t nImageFormat, uint32_t nWidth, uint32_t nHeight, bool bRenderable, bool bMappable, bool bComputeAccess, uint32_t unMipLevels, uint32_t unArrayLayerCount, SharedTextureHandle_t *pSharedHandle ) = 0;

	/** Create a new tracked Vulkan Buffer */
	virtual bool NewSharedVulkanBuffer( uint32_t nSize, uint32_t nUsageFlags, SharedTextureHandle_t *pSharedHandle ) = 0;

	/** Create a new tracked Vulkan Semaphore */
	virtual bool NewSharedVulkanSemaphore( bool bCounting, SharedTextureHandle_t *pSharedHandle ) = 0;

	/** Grab a reference to hSharedHandle, and optionally generate a new IPC handle if pNewIpcHandle is not nullptr  */
	virtual bool RefResource( SharedTextureHandle_t hSharedHandle, uint64_t *pNewIpcHandle ) = 0;

	/** Drop a reference to hSharedHandle */
	virtual bool UnrefResource( SharedTextureHandle_t hSharedHandle ) = 0;

	/* Get all the DRM formats we support using DMA-BUF images for.
	 *
	 * pOutFormatCount and pOutFormats function like Vulkan:
	 *   - If pOutFormats is NULL, then pOutFormatCount will be overwritten with the format count.
	 *   - If pOutFormats is not NULL, then pOutFormatCount specifies the size of the pOutFormats array,
	 *       and will be overwritten with the number of formats written to the array.
	 *
	 * If the function fails, false is returned, and pOutFormatCount will be 0.
	 * Supported on Linux only.
	 */
	virtual bool GetDmabufFormats( uint32_t *pOutFormatCount, uint32_t *pOutFormats ) = 0;

	/** Get dmabuf modifiers we are allowed to use.
	 *
	 * pOutModifierCount and pOutModifiers function like Vulkan:
	 *   - If pOutModifiers is NULL, then pOutModifierCount will be overwritten with the modifier count.
	 *   - If pOutModifiers is not NULL, then pOutModifierCount specifies the size of the pOutModifiers array,
	 *       and will be overwritten with the number of modifiers written to the array.
	 *
	 * If modifiers are not supported, a single DRM_FORMAT_MOD_INVALID entry will be returned.
	 *
	 * If the function fails, false is returned, and pOutModifierCount will be 0.
	 * Supported on Linux only.
	 */
	virtual bool GetDmabufModifiers( EVRApplicationType eApplicationType, uint32_t unDRMFormat, uint32_t *pOutModifierCount, uint64_t *pOutModifiers ) = 0;

	/** Import a dmabuf directly.
	 * Note: the FD you pass in will be dup'ed, so you must close it yourself.
	 * This function does NOT take ownership of the fd you pass in.
	 * Supported on Linux only.
	 */
	virtual bool ImportDmabuf( EVRApplicationType eApplicationType, DmabufAttributes_t *pDmabufAttributes, SharedTextureHandle_t *pSharedHandle ) = 0;

	/** Consumes an IPC handle (eg. from RefResource) and returns a file-descriptor.
	 * Caller acquires ownership of fd and is responsible for closing it.
	 * Supported on Linux only.
	 */
	virtual bool ReceiveSharedFd( uint64_t ulIpcHandle, int *pOutFd ) = 0;

protected:
	/** Non-deletable */
	virtual ~IVRIPCResourceManagerClient() {};
};

static const char *IVRIPCResourceManagerClient_Version = "IVRIPCResourceManagerClient_002";

}
// End




namespace vr
{
namespace IVRIPCResourceManagerClient_002
{
#if !defined( OPENVR_INTERFACE_INTERNAL )

	/** Finds the active installation of the VR API and initializes it. The provided path must be absolute
	* or relative to the current working directory. These are the local install versions of the equivalent
	* functions in steamvr.h and will work without a local Steam install.
	*
	* This path is to the "root" of the VR API install. That's the directory with
	* the "drivers" directory and a platform (i.e. "win32") directory in it, not the directory with the DLL itself.
	*
	* pStartupInfo is reserved for future use.
	*/
	inline IVRSystem *VR_Init( EVRInitError *peError, EVRApplicationType eApplicationType, const char *pStartupInfo = nullptr );

	/** unloads vrclient.dll. Any interface pointers from the interface are
	* invalid after this point */
	inline void VR_Shutdown();

	/** Returns true if there is an HMD attached. This check is as lightweight as possible and
	* can be called outside of VR_Init/VR_Shutdown. It should be used when an application wants
	* to know if initializing VR is a possibility but isn't ready to take that step yet.
	*/
	VR_INTERFACE bool VR_CALLTYPE VR_IsHmdPresent();

	/** Returns true if the OpenVR runtime is installed. */
	VR_INTERFACE bool VR_CALLTYPE VR_IsRuntimeInstalled();

	/** Returns where the OpenVR runtime is installed. */
	VR_INTERFACE bool VR_GetRuntimePath( VR_OUT_STRING() char *pchPathBuffer, uint32_t unBufferSize, uint32_t *punRequiredBufferSize );

	/** Returns the name of the enum value for an EVRInitError. This function may be called outside of VR_Init()/VR_Shutdown(). */
	VR_INTERFACE const char *VR_CALLTYPE VR_GetVRInitErrorAsSymbol( EVRInitError error );

	/** Returns an English string for an EVRInitError. Applications should call VR_GetVRInitErrorAsSymbol instead and
	* use that as a key to look up their own localized error message. This function may be called outside of VR_Init()/VR_Shutdown(). */
	VR_INTERFACE const char *VR_CALLTYPE VR_GetVRInitErrorAsEnglishDescription( EVRInitError error );

	/** Returns the interface of the specified version. This method must be called after VR_Init. The
	* pointer returned is valid until VR_Shutdown is called.
	*/
	VR_INTERFACE void *VR_CALLTYPE VR_GetGenericInterface( const char *pchInterfaceVersion, EVRInitError *peError );

	/** Returns whether the interface of the specified version exists.
	*/
	VR_INTERFACE bool VR_CALLTYPE VR_IsInterfaceVersionValid( const char *pchInterfaceVersion );

	/** Returns a token that represents whether the VR interface handles need to be reloaded */
	VR_INTERFACE uint32_t VR_CALLTYPE VR_GetInitToken();

	// These typedefs allow old enum names from SDK 0.9.11 to be used in applications.
	// They will go away in the future.
	typedef EVRInitError HmdError;
	typedef EVREye Hmd_Eye;
	typedef EColorSpace ColorSpace;
	typedef ETrackingResult HmdTrackingResult;
	typedef ETrackedDeviceClass TrackedDeviceClass;
	typedef ETrackingUniverseOrigin TrackingUniverseOrigin;
	typedef ETrackedDeviceProperty TrackedDeviceProperty;
	typedef ETrackedPropertyError TrackedPropertyError;
	typedef EVRSubmitFlags VRSubmitFlags_t;
	typedef EVRState VRState_t;
	typedef ECollisionBoundsStyle CollisionBoundsStyle_t;
	typedef EVROverlayError VROverlayError;
	typedef EVRFirmwareError VRFirmwareError;
	typedef EVRCompositorError VRCompositorError;
	typedef EVRScreenshotError VRScreenshotsError;

	inline uint32_t &VRToken()
	{
		static uint32_t token;
		return token;
	}

	class COpenVRContext
	{
	public:
		COpenVRContext() { Clear(); }
		void Clear();

		inline void CheckClear()
		{
			if ( VRToken() != VR_GetInitToken() )
			{
				Clear();
				VRToken() = VR_GetInitToken();
			}
		}

		IVRSystem *VRSystem()
		{
			CheckClear();
			if ( m_pVRSystem == nullptr )
			{
				EVRInitError eError;
				m_pVRSystem = ( IVRSystem * )VR_GetGenericInterface( IVRSystem_Version, &eError );
			}
			return m_pVRSystem;
		}
		IVRChaperone *VRChaperone()
		{
			CheckClear();
			if ( m_pVRChaperone == nullptr )
			{
				EVRInitError eError;
				m_pVRChaperone = ( IVRChaperone * )VR_GetGenericInterface( IVRChaperone_Version, &eError );
			}
			return m_pVRChaperone;
		}

		IVRChaperoneSetup *VRChaperoneSetup()
		{
			CheckClear();
			if ( m_pVRChaperoneSetup == nullptr )
			{
				EVRInitError eError;
				m_pVRChaperoneSetup = ( IVRChaperoneSetup * )VR_GetGenericInterface( IVRChaperoneSetup_Version, &eError );
			}
			return m_pVRChaperoneSetup;
		}

		IVRCompositor *VRCompositor()
		{
			CheckClear();
			if ( m_pVRCompositor == nullptr )
			{
				EVRInitError eError;
				m_pVRCompositor = ( IVRCompositor * )VR_GetGenericInterface( IVRCompositor_Version, &eError );
			}
			return m_pVRCompositor;
		}

		IVROverlay *VROverlay()
		{
			CheckClear();
			if ( m_pVROverlay == nullptr )
			{
				EVRInitError eError;
				m_pVROverlay = ( IVROverlay * )VR_GetGenericInterface( IVROverlay_Version, &eError );
			}
			return m_pVROverlay;
		}

		IVROverlayView *VROverlayView()
		{
			CheckClear();
			if ( m_pVROverlayView == nullptr )
			{
				EVRInitError eError;
				m_pVROverlayView = ( IVROverlayView * ) VR_GetGenericInterface( IVROverlayView_Version, &eError );
			}
			return m_pVROverlayView;
		}

		IVRHeadsetView *VRHeadsetView()
		{
			CheckClear();
			if ( m_pVRHeadsetView == nullptr )
			{
				EVRInitError eError;
				m_pVRHeadsetView = ( IVRHeadsetView * ) VR_GetGenericInterface( IVRHeadsetView_Version, &eError );
			}
			return m_pVRHeadsetView;
		}

		IVRResources *VRResources()
		{
			CheckClear();
			if ( m_pVRResources == nullptr )
			{
				EVRInitError eError;
				m_pVRResources = (IVRResources *)VR_GetGenericInterface( IVRResources_Version, &eError );
			}
			return m_pVRResources;
		}

		IVRScreenshots *VRScreenshots()
		{
			CheckClear();
			if ( m_pVRScreenshots == nullptr )
			{
				EVRInitError eError;
				m_pVRScreenshots = ( IVRScreenshots * )VR_GetGenericInterface( IVRScreenshots_Version, &eError );
			}
			return m_pVRScreenshots;
		}

		IVRRenderModels *VRRenderModels()
		{
			CheckClear();
			if ( m_pVRRenderModels == nullptr )
			{
				EVRInitError eError;
				m_pVRRenderModels = ( IVRRenderModels * )VR_GetGenericInterface( IVRRenderModels_Version, &eError );
			}
			return m_pVRRenderModels;
		}

		IVRExtendedDisplay *VRExtendedDisplay()
		{
			CheckClear();
			if ( m_pVRExtendedDisplay == nullptr )
			{
				EVRInitError eError;
				m_pVRExtendedDisplay = ( IVRExtendedDisplay * )VR_GetGenericInterface( IVRExtendedDisplay_Version, &eError );
			}
			return m_pVRExtendedDisplay;
		}

		IVRSettings *VRSettings()
		{
			CheckClear();
			if ( m_pVRSettings == nullptr )
			{
				EVRInitError eError;
				m_pVRSettings = ( IVRSettings * )VR_GetGenericInterface( IVRSettings_Version, &eError );
			}
			return m_pVRSettings;
		}

		IVRApplications *VRApplications()
		{
			CheckClear();
			if ( m_pVRApplications == nullptr )
			{
				EVRInitError eError;
				m_pVRApplications = ( IVRApplications * )VR_GetGenericInterface( IVRApplications_Version, &eError );
			}
			return m_pVRApplications;
		}

		IVRTrackedCamera *VRTrackedCamera()
		{
			CheckClear();
			if ( m_pVRTrackedCamera == nullptr )
			{
				EVRInitError eError;
				m_pVRTrackedCamera = ( IVRTrackedCamera * )VR_GetGenericInterface( IVRTrackedCamera_Version, &eError );
			}
			return m_pVRTrackedCamera;
		}

		IVRDriverManager *VRDriverManager()
		{
			CheckClear();
			if ( !m_pVRDriverManager )
			{
				EVRInitError eError;
				m_pVRDriverManager = ( IVRDriverManager * )VR_GetGenericInterface( IVRDriverManager_Version, &eError );
			}
			return m_pVRDriverManager;
		}

		IVRInput *VRInput()
		{
			CheckClear();
			if ( !m_pVRInput )
			{
				EVRInitError eError;
				m_pVRInput = (IVRInput *)VR_GetGenericInterface( IVRInput_Version, &eError );
			}
			return m_pVRInput;
		}

		IVRIOBuffer *VRIOBuffer()
		{
			if ( !m_pVRIOBuffer )
			{
				EVRInitError eError;
				m_pVRIOBuffer = ( IVRIOBuffer * )VR_GetGenericInterface( IVRIOBuffer_Version, &eError );
			}
			return m_pVRIOBuffer;
		}

		IVRSpatialAnchors *VRSpatialAnchors()
		{
			CheckClear();
			if ( !m_pVRSpatialAnchors )
			{
				EVRInitError eError;
				m_pVRSpatialAnchors = (IVRSpatialAnchors *)VR_GetGenericInterface( IVRSpatialAnchors_Version, &eError );
			}
			return m_pVRSpatialAnchors;
		}

		IVRDebug *VRDebug()
		{
			CheckClear();
			if ( !m_pVRDebug )
			{
				EVRInitError eError;
				m_pVRDebug = (IVRDebug *)VR_GetGenericInterface( IVRDebug_Version, &eError );
			}
			return m_pVRDebug;
		}

		IVRNotifications *VRNotifications()
		{
			CheckClear();
			if ( !m_pVRNotifications )
			{
				EVRInitError eError;
				m_pVRNotifications = ( IVRNotifications * )VR_GetGenericInterface( IVRNotifications_Version, &eError );
			}
			return m_pVRNotifications;
		}

		IVRIPCResourceManagerClient *VRIPCResourceManager()
		{
			CheckClear();
			if ( !m_pVRIPCResourceManagerClient )
			{
				EVRInitError eError;
				m_pVRIPCResourceManagerClient = ( IVRIPCResourceManagerClient * )VR_GetGenericInterface( IVRIPCResourceManagerClient_Version, &eError );
			}
			return m_pVRIPCResourceManagerClient;
		}

	private:
		IVRSystem			*m_pVRSystem;
		IVRChaperone		*m_pVRChaperone;
		IVRChaperoneSetup	*m_pVRChaperoneSetup;
		IVRCompositor		*m_pVRCompositor;
		IVRHeadsetView		*m_pVRHeadsetView;
		IVROverlay			*m_pVROverlay;
		IVROverlayView		*m_pVROverlayView;
		IVRResources		*m_pVRResources;
		IVRRenderModels		*m_pVRRenderModels;
		IVRExtendedDisplay	*m_pVRExtendedDisplay;
		IVRSettings			*m_pVRSettings;
		IVRApplications		*m_pVRApplications;
		IVRTrackedCamera	*m_pVRTrackedCamera;
		IVRScreenshots		*m_pVRScreenshots;
		IVRDriverManager	*m_pVRDriverManager;
		IVRInput			*m_pVRInput;
		IVRIOBuffer			*m_pVRIOBuffer;
		IVRSpatialAnchors   *m_pVRSpatialAnchors;
		IVRDebug			*m_pVRDebug;
		IVRNotifications	*m_pVRNotifications;
		IVRIPCResourceManagerClient *m_pVRIPCResourceManagerClient;
	};

	inline COpenVRContext &OpenVRInternal_ModuleContext()
	{
		static void *ctx[ sizeof( COpenVRContext ) / sizeof( void * ) ];
		return *( COpenVRContext * )ctx; // bypass zero-init constructor
	}

	inline IVRSystem *VR_CALLTYPE VRSystem() { return OpenVRInternal_ModuleContext().VRSystem(); }
	inline IVRChaperone *VR_CALLTYPE VRChaperone() { return OpenVRInternal_ModuleContext().VRChaperone(); }
	inline IVRChaperoneSetup *VR_CALLTYPE VRChaperoneSetup() { return OpenVRInternal_ModuleContext().VRChaperoneSetup(); }
	inline IVRCompositor *VR_CALLTYPE VRCompositor() { return OpenVRInternal_ModuleContext().VRCompositor(); }
	inline IVROverlay *VR_CALLTYPE VROverlay() { return OpenVRInternal_ModuleContext().VROverlay(); }
	inline IVROverlayView *VR_CALLTYPE VROverlayView() { return OpenVRInternal_ModuleContext().VROverlayView(); }
	inline IVRHeadsetView *VR_CALLTYPE VRHeadsetView() { return OpenVRInternal_ModuleContext().VRHeadsetView(); }
	inline IVRScreenshots *VR_CALLTYPE VRScreenshots() { return OpenVRInternal_ModuleContext().VRScreenshots(); }
	inline IVRRenderModels *VR_CALLTYPE VRRenderModels() { return OpenVRInternal_ModuleContext().VRRenderModels(); }
	inline IVRApplications *VR_CALLTYPE VRApplications() { return OpenVRInternal_ModuleContext().VRApplications(); }
	inline IVRSettings *VR_CALLTYPE VRSettings() { return OpenVRInternal_ModuleContext().VRSettings(); }
	inline IVRResources *VR_CALLTYPE VRResources() { return OpenVRInternal_ModuleContext().VRResources(); }
	inline IVRExtendedDisplay *VR_CALLTYPE VRExtendedDisplay() { return OpenVRInternal_ModuleContext().VRExtendedDisplay(); }
	inline IVRTrackedCamera *VR_CALLTYPE VRTrackedCamera() { return OpenVRInternal_ModuleContext().VRTrackedCamera(); }
	inline IVRDriverManager *VR_CALLTYPE VRDriverManager() { return OpenVRInternal_ModuleContext().VRDriverManager(); }
	inline IVRInput *VR_CALLTYPE VRInput() { return OpenVRInternal_ModuleContext().VRInput(); }
	inline IVRIOBuffer *VR_CALLTYPE VRIOBuffer() { return OpenVRInternal_ModuleContext().VRIOBuffer(); }
	inline IVRSpatialAnchors *VR_CALLTYPE VRSpatialAnchors() { return OpenVRInternal_ModuleContext().VRSpatialAnchors(); }
	inline IVRNotifications *VR_CALLTYPE VRNotifications() { return OpenVRInternal_ModuleContext().VRNotifications(); }
	inline IVRDebug *VR_CALLTYPE VRDebug() { return OpenVRInternal_ModuleContext().VRDebug(); }
	inline IVRIPCResourceManagerClient *VR_CALLTYPE VRIPCResourceManager() { return OpenVRInternal_ModuleContext().VRIPCResourceManager(); }

	inline void COpenVRContext::Clear()
	{
		m_pVRSystem = nullptr;
		m_pVRChaperone = nullptr;
		m_pVRChaperoneSetup = nullptr;
		m_pVRCompositor = nullptr;
		m_pVROverlay = nullptr;
		m_pVROverlayView = nullptr;
		m_pVRHeadsetView = nullptr;
		m_pVRRenderModels = nullptr;
		m_pVRExtendedDisplay = nullptr;
		m_pVRSettings = nullptr;
		m_pVRApplications = nullptr;
		m_pVRTrackedCamera = nullptr;
		m_pVRResources = nullptr;
		m_pVRScreenshots = nullptr;
		m_pVRDriverManager = nullptr;
		m_pVRInput = nullptr;
		m_pVRIOBuffer = nullptr;
		m_pVRSpatialAnchors = nullptr;
		m_pVRNotifications = nullptr;
		m_pVRDebug = nullptr;
		m_pVRIPCResourceManagerClient = nullptr;
	}

	VR_INTERFACE uint32_t VR_CALLTYPE VR_InitInternal2( EVRInitError *peError, EVRApplicationType eApplicationType, const char *pStartupInfo );
	VR_INTERFACE void VR_CALLTYPE VR_ShutdownInternal();

	/** Finds the active installation of vrclient.dll and initializes it */
	inline IVRSystem *VR_Init( EVRInitError *peError, EVRApplicationType eApplicationType, const char *pStartupInfo )
	{
		IVRSystem *pVRSystem = nullptr;

		EVRInitError eError;
		VRToken() = VR_InitInternal2( &eError, eApplicationType, pStartupInfo );
		COpenVRContext &ctx = OpenVRInternal_ModuleContext();
		ctx.Clear();

		if ( eError == VRInitError_None )
		{
			if ( VR_IsInterfaceVersionValid( IVRSystem_Version ) )
			{
				pVRSystem = VRSystem();
			}
			else
			{
				VR_ShutdownInternal();
				eError = VRInitError_Init_InterfaceNotFound;
			}
		}

		if ( peError )
			*peError = eError;
		return pVRSystem;
	}

	/** unloads vrclient.dll. Any interface pointers from the interface are
	* invalid after this point */
	inline void VR_Shutdown()
	{
		VR_ShutdownInternal();
	}

#endif // OPENVR_INTERFACE_INTERNAL
}} // Close custom namespace
