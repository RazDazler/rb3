#include "network/ObjDup/DuplicatedObject.h"
#include "Core/Scheduler.h"
#include "Core/StateMachine.h"
#include "ObjDup/DOOperation.h"
#include "Platform/CriticalSection.h"
#include "Platform/ScopedCS.h"
#include "Platform/SystemError.h"

namespace Quazal {

    CriticalSection DuplicatedObject::s_csRefCount(0x40000000);

    DuplicatedObject::DuplicatedObject()
        : StateMachine(static_cast<StateFunc>(&DuplicatedObject::SetInitialState)),
          m_setDuplicationSet(3), m_setCachedDuplicationSet(0) {
        m_uiRefCount = 0;
        m_uiRelevanceCount = 0;
        m_uiFlags = 0;
        {
            ScopedCS cs(Scheduler::GetInstance()->unk38);
            AcquireMainReference();
            m_uiFlags |= 1;
        }
        InitialTransition();
    }

    DuplicatedObject::~DuplicatedObject() {}

    bool DuplicatedObject::SpecificRefresh(DataSet *dataset, const Time &) {
        if (!dataset)
            return true;
        SystemError::SignalError(0, 0, 0xe0000016, 0);
        return false;
    }
    bool DuplicatedObject::SpecificUpdate(DataSet *dataset, Time) {
        if (!dataset)
            return true;
        SystemError::SignalError(0, 0, 0xe0000016, 0);
        return false;
    }
    bool DuplicatedObject::IsAWellKnownDO() const { return m_dohMyself.IsAWKHandle(); }
    bool DuplicatedObject::CallApproveFaultRecovery() {
        if (m_dohMyself.IsAWKHandle())
            return true;
        return ApproveFaultRecovery();
    }
    bool DuplicatedObject::IsADuplica() const {
        if (m_refMasterStation.m_hReferencedDO.mValue == DOHandle().mValue)
            return false;
        else
            return !IsADuplicationMaster();
    }

    bool DuplicatedObject::HasGlobalDOProperty() const { return false; }
    bool DuplicatedObject::HasForcedNonGlobalProperty() const { return false; }
    bool DuplicatedObject::ApproveFaultRecovery() { return false; }
    bool DuplicatedObject::ApproveEmigration(unsigned int) { return false; }
    void DuplicatedObject::InitDO() {}
    void DuplicatedObject::TestInvariants() {}
    void DuplicatedObject::CallOperationEndOnAdapters(DOOperation *) {}
    void DuplicatedObject::CallOperationOnDatasets(DOOperation *, Operation::_Event) {}
    void DuplicatedObject::ClearFlag(unsigned short mask) { m_uiFlags &= mask ^ 0xffff; }

    void DuplicatedObject::SetStationSpecialRelevance() {
        m_refMasterStation.SetSoft();
        m_setDuplicationSet.SetFlags(1);
    }

    void DuplicatedObject::OperationBegin(DOOperation *) {}
    void DuplicatedObject::OperationEnd(DOOperation *) {}
    float DuplicatedObject::ComputeDistance(DuplicatedObject *) { return -1; }
    void DuplicatedObject::ReleaseReferenceToMaster() { m_refMasterStation.Release(); }

    void DuplicatedObject::AcquireReferenceToMaster() {
        if (!m_refMasterStation.m_poReferencedDO) {
            m_refMasterStation.Acquire();
        }
    }

    bool DuplicatedObject::IsInDuplicationSet(DOHandle h) const {
        return m_setDuplicationSet.m_map.find(h) != m_setDuplicationSet.m_map.end();
    }

    void DuplicatedObject::SetInitialState(const QEvent &) {
        mCurrentState = reinterpret_cast<StateFuncFactory>(&DuplicatedObject::ValidState);
    }

    StateMachine::StateFuncFactory DuplicatedObject::InvalidState(const QEvent &e) {
        return static_cast<StateFuncFactory>(&StateMachine::TopState);
    }

    StateMachine::StateFuncFactory DuplicatedObject::ValidState(const QEvent &e) {
        if ((int)e.GetSignal() == 1) {
            mCurrentState =
                reinterpret_cast<StateFuncFactory>(&DuplicatedObject::ValidState);
            return 0;
        } else if ((((unsigned int)e.GetSignal() - 4) >> 31) == 0) {
            static_cast<const Operation &>(e).Trace(1);
            Trace(1);
            static TransitionPath t_;
            StaticStateTransition(
                &t_, reinterpret_cast<StateFuncFactory>(&DuplicatedObject::InvalidState)
            );
            return 0;
        } else {
            return static_cast<StateFuncFactory>(&StateMachine::TopState);
        }
    }

}