#pragma once
#include "BaseCommon.h"
#include "interfaces/IVRHeadsetView_001.h"
#include "Reimpl/BaseHeadsetView.h"
class CVRHeadsetView_001 : public vr::IVRHeadsetView_001::IVRHeadsetView, public CVRCommon {
private:
	const std::shared_ptr<BaseHeadsetView> base;
public:
	virtual void** _GetStatFuncList() override;
	virtual void Delete() override;
	CVRHeadsetView_001();
	// Interface methods:
	void SetHeadsetViewSize(uint32_t nWidth, uint32_t nHeight) override;
	void GetHeadsetViewSize(uint32_t* pnWidth, uint32_t* pnHeight) override;
	void SetHeadsetViewMode(vr::IVRHeadsetView_001::HeadsetViewMode_t eHeadsetViewMode) override;
	vr::IVRHeadsetView_001::HeadsetViewMode_t GetHeadsetViewMode() override;
	void SetHeadsetViewCropped(bool bCropped) override;
	bool GetHeadsetViewCropped() override;
	float GetHeadsetViewAspectRatio() override;
	void SetHeadsetViewBlendRange(float flStartPct, float flEndPct) override;
	void GetHeadsetViewBlendRange(float* pStartPct, float* pEndPct) override;
};
