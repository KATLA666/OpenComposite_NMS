#pragma once
#include "openvr.h"
#include "version.h"
// public_vrtypes.h


namespace vr
{
#pragma pack( push, 8 )

typedef uint32_t PropertyTypeTag_t;

// right-handed system
// +y is up
// +x is to the right
// -z is forward
// Distance unit is  meters
struct HmdMatrix34_t
{
	float m[3][4];
};

struct HmdMatrix33_t
{
	float m[3][3];
};

struct HmdMatrix44_t
{
	float m[4][4];
};

struct HmdVector3_t
{
	float v[3];
};

struct HmdVector4_t
{
	float v[4];
};

struct HmdVector3d_t
{
	double v[3];
};

struct HmdVector2_t
{
	float v[2];
};

struct HmdQuaternion_t
{
	double w, x, y, z;
};

struct HmdQuaternionf_t
{
	float w, x, y, z;
};

struct HmdColor_t
{
	float r, g, b, a;
};

struct HmdQuad_t
{
	HmdVector3_t vCorners[ 4 ];
};

struct HmdRect2_t
{
	HmdVector2_t vTopLeft;
	HmdVector2_t vBottomRight;
};

/** Holds the transform for a single bone */
struct VRBoneTransform_t
{
	HmdVector4_t position;
	HmdQuaternionf_t orientation;
};

struct VREyeTrackingData_t
{
	bool bActive;
	bool bValid;
	bool bTracked;

	vr::HmdVector3_t vGazeOrigin;  // Ray origin
	vr::HmdVector3_t vGazeTarget;  // Gaze target (fixation point)
};

/** Used to return the post-distortion UVs for each color channel.
* UVs range from 0 to 1 with 0,0 in the upper left corner of the
* source render target. The 0,0 to 1,1 range covers a single eye. */
struct DistortionCoordinates_t
{
	float rfRed[2];
	float rfGreen[2];
	float rfBlue[2];
};

enum EVREye
{
	Eye_Left = 0,
	Eye_Right = 1
};

enum ETextureType
{
	TextureType_Invalid = -1, // Handle has been invalidated
	TextureType_DirectX = 0, // Handle is an ID3D11Texture
	TextureType_OpenGL = 1,  // Handle is an OpenGL texture name or an OpenGL render buffer name, depending on submit flags
	TextureType_Vulkan = 2, // Handle is a pointer to a VRVulkanTextureData_t structure
	TextureType_IOSurface = 3, // Handle is a macOS cross-process-sharable IOSurfaceRef, deprecated in favor of TextureType_Metal on supported platforms
	TextureType_DirectX12 = 4, // Handle is a pointer to a D3D12TextureData_t structure
	TextureType_DXGISharedHandle = 5, // Handle is a HANDLE DXGI share handle, only supported for Overlay render targets.
									  // this texture is used directly by our renderer, so only perform atomic (copyresource or resolve) on it
	TextureType_Metal = 6,	// Handle is a MTLTexture conforming to the MTLSharedTexture protocol. Textures submitted to IVRCompositor::Submit which
							// are of type MTLTextureType2DArray assume layer 0 is the left eye texture (vr::EVREye::Eye_left), layer 1 is the right
							// eye texture (vr::EVREye::Eye_Right)

	TextureType_Reserved = 7,
	TextureType_SharedTextureHandle = 8, // A pointer to a vr::SharedTextureHandle_t that was imported via, eg. ImportDmabuf.
};

enum EColorSpace
{
	ColorSpace_Auto = 0,	// Assumes 'gamma' for 8-bit per component formats, otherwise 'linear'.  This mirrors the DXGI formats which have _SRGB variants.
	ColorSpace_Gamma = 1,	// Texture data can be displayed directly on the display without any conversion (a.k.a. display native format).
	ColorSpace_Linear = 2,	// Same as gamma but has been converted to a linear representation using DXGI's sRGB conversion algorithm.
};

struct Texture_t
{
	void* handle; // See ETextureType definition above
	ETextureType eType;
	EColorSpace eColorSpace;
};

/** Allows the application to control what part of the provided texture will be used in the
* frame buffer. */
struct VRTextureBounds_t
{
	float uMin, vMin;
	float uMax, vMax;
};

/** Allows specifying pose used to render provided scene texture (if different from value returned by WaitGetPoses). */
struct VRTextureWithPose_t : public Texture_t
{
	HmdMatrix34_t mDeviceToAbsoluteTracking; // Actual pose used to render scene textures.
};

struct VRTextureDepthInfo_t
{
	void* handle; // See ETextureType definition above
	HmdMatrix44_t mProjection;
	HmdVector2_t vRange; // 0..1
};

struct VRTextureWithDepth_t : public Texture_t
{
	VRTextureDepthInfo_t depth;
};

struct VRTextureWithPoseAndDepth_t : public VRTextureWithPose_t
{
	VRTextureDepthInfo_t depth;
};

struct VRTextureMotionInfo_t
{
	void *handle; // See ETextureType definition above
	HmdMatrix44_t mDeltaPose; // Incremental application-applied transform, if any, since the previous frame that affects the view.
};

struct VRTextureWithMotion_t : VRTextureWithPoseAndDepth_t
{
	VRTextureMotionInfo_t motion;
};

// 64-bit types that are part of public structures
// that are replicated in shared memory.
#if defined(__linux__) || defined(__APPLE__)
typedef uint64_t vrshared_uint64_t __attribute__ ((aligned(8)));
typedef double vrshared_double __attribute__ ((aligned(8)));
#else
typedef uint64_t vrshared_uint64_t;
typedef double vrshared_double;
#endif

static const uint32_t MaxDmabufPlaneCount = 4;

struct DmabufPlane_t
{
	uint32_t unOffset;
	uint32_t unStride;
	int32_t nFd; // This is not consumed, it is dup'ed.
};

struct DmabufAttributes_t
{
	void *pNext; // MUST be NULL. Unused right now, but could be used to extend this structure in the future.

	uint32_t unWidth;
	uint32_t unHeight;
	uint32_t unDepth;
	uint32_t unMipLevels;
	uint32_t unArrayLayers;
	uint32_t unSampleCount;
	uint32_t unFormat;   // DRM_FORMAT_
	uint64_t ulModifier; // DRM_FORMAT_MOD_

	uint32_t unPlaneCount;
	DmabufPlane_t plane[MaxDmabufPlaneCount];
};

#pragma pack( pop )

} // namespace vr

// Uncommented include guard ifdef was here, removed while splitting

