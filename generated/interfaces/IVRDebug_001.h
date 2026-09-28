#pragma once
#include "openvr.h"
#include "version.h"
#include "public_vrtypes.h"
#include "vrtypes.h"
#include "vrannotation.h"
// ivrdebug.h

namespace vr
{
namespace IVRDebug_001
{
	enum EVRDebugError
	{
		VRDebugError_Success = 0,
		VRDebugError_BadParameter
	};

	/** Handle for vr profiler events */
	typedef uint64_t VrProfilerEventHandle_t;

	class IVRDebug
	{
	public:

		/** Create a vr profiler discrete event (point)
		* The event will be associated with the message provided in pchMessage, and the current
		* time will be used as the event timestamp. */
		virtual EVRDebugError EmitVrProfilerEvent( const char *pchMessage ) = 0;

		/** Create an vr profiler duration event (line)
		* The current time will be used as the timestamp for the start of the line.
		* On success, pHandleOut will contain a handle valid for terminating this event. */
		virtual EVRDebugError BeginVrProfilerEvent( VrProfilerEventHandle_t *pHandleOut ) = 0;

		/** Terminate a vr profiler event
		* The event associated with hHandle will be considered completed when this method is called.
		* The current time will be used assocaited to the termination time of the event, and
		* pchMessage will be used as the event title. */
		virtual EVRDebugError FinishVrProfilerEvent( VrProfilerEventHandle_t hHandle, const char *pchMessage ) = 0;

		/** Sends a request to the driver for the specified device and returns the response. The maximum response size is 32k,
		* but this method can be called with a smaller buffer. If the response exceeds the size of the buffer, it is truncated.
		* The size of the response including its terminating null is returned. */
		virtual uint32_t DriverDebugRequest( TrackedDeviceIndex_t unDeviceIndex, const char *pchRequest, VR_OUT_STRING() char *pchResponseBuffer, uint32_t unResponseBufferSize ) = 0;

	};

	static const char * const IVRDebug_Version = "IVRDebug_001";

} // namespace vr

} // Close custom namespace
