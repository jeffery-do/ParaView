/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id$  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2007 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#include "XdmfDsm.h"
#include "XdmfDsmComm.h"
#include "XdmfDsmMsg.h"
#include "XdmfArray.h"

// Align
typedef struct {
    XdmfInt64   Opcode;
    XdmfInt64   Source;
    XdmfInt64   Target;
    XdmfInt64   Address;
    XdmfInt64   Length;
    XdmfInt64   Parameters[10];
} XdmfDsmCommand;

XdmfDsm::XdmfDsm() {
    this->DsmType = XDMF_DSM_TYPE_UNIFORM;
    this->Storage = new XdmfArray;
    // For Alignment
    this->Storage->SetNumberType(XDMF_INT64_TYPE);
    this->SetLength(XDMF_DSM_DEFAULT_LENGTH);
    this->StartAddress = 0;
    this->EndAddress = this->StartAddress + this->Length - 1;
    this->Comm = 0;
    this->StartServerId = this->EndServerId = -1;
    this->Msg = new XdmfDsmMsg;
}

XdmfDsm::~XdmfDsm() {
    if(this->Storage) delete this->Storage;
}

XdmfInt32
XdmfDsm::ConfigureUniform(XdmfDsmComm *Comm, XdmfInt64 Length, XdmfInt32 StartId, XdmfInt32 EndId){
    if(StartId < 0) StartId = 0;
    if(EndId < 0) EndId = Comm->GetTotalSize() - 1;
    // cout << "(" << Comm->GetId() << ") StartId " << StartId << " EndId " << EndId << endl;
    this->SetDsmType(XDMF_DSM_TYPE_UNIFORM_RANGE);
    if((StartId == 0) && (EndId == Comm->GetTotalSize() - 1)){
        this->SetDsmType(XDMF_DSM_TYPE_UNIFORM);
    }
    this->SetStartServerId(StartId);
    this->SetEndServerId(EndId);
    this->SetComm(Comm);
    if((Comm->GetId() >= StartId) && (Comm->GetId() <= EndId)){
        this->SetLength(Length);
        this->StartAddress = (Comm->GetId() - StartId) * Length;
        this->EndAddress = this->StartAddress + Length - 1;
    }else{
        this->Length = Length;
    }
    // cout << "(" << Comm->GetId() << ") start " << this->StartAddress << " end " << this->EndAddress << endl;
    this->Msg->Source = this->Comm->GetId();
    this->TotalLength = Length * (EndId - StartId + 1);
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsm::GetAddressRangeForId(XdmfInt32 Id, XdmfInt64 *Start, XdmfInt64 *End){
    switch(this->DsmType) {
        case XDMF_DSM_TYPE_UNIFORM :
        case XDMF_DSM_TYPE_UNIFORM_RANGE :
            // All Servers have same length
            *Start = (Id - this->StartServerId) * this->Length;
            *End = *Start + Length - 1;
            break;
        default :
            // Not Implemented
            XdmfErrorMessage("DsmType " << this->DsmType << " not yet implemented");
            return(XDMF_FAIL);
            break;
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsm::AddressToId(XdmfInt64 Address){
    XdmfInt32   ServerId = XDMF_FAIL;

    switch(this->DsmType) {
        case XDMF_DSM_TYPE_UNIFORM :
        case XDMF_DSM_TYPE_UNIFORM_RANGE :
            // All Servers have same length
            ServerId = this->StartServerId + (Address / this->Length);
            if(ServerId > this->EndServerId ){
                XdmfErrorMessage("ServerId " << ServerId << " for Address " << Address << " is larger than EndServerId " << this->EndServerId);
            }
            break;
        default :
            // Not Implemented
            XdmfErrorMessage("DsmType " << this->DsmType << " not yet implemented");
            break;
    }
    return(ServerId);
}

XdmfInt32
XdmfDsm::SendDone(){
    XdmfInt32   who, status = XDMF_SUCCESS;

    switch(this->DsmType) {
        case XDMF_DSM_TYPE_UNIFORM :
        case XDMF_DSM_TYPE_UNIFORM_RANGE :
            for(who = this->StartServerId ; who <= this->EndServerId ; who++){
                status = this->SendCommandHeader(XDMF_DSM_OPCODE_DONE, who, 0, 0);
            }
            break;
        default :
            // Not Implemented
            XdmfErrorMessage("DsmType " << this->DsmType << " not yet implemented");
            break;
    }
    return(status);
}

XdmfInt32
XdmfDsm::SetLength(XdmfInt64 Length){
    // Make it longer than actually needed for round off.
    if(this->Storage->SetNumberOfElements((Length / sizeof(XdmfInt64)) + 1) != XDMF_SUCCESS){
        XdmfErrorMessage("Cannot set Dsm Length to " << Length);
        return(XDMF_FAIL);
    }
    this->Length = Length;
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsm::SendCommandHeader(XdmfInt32 Opcode, XdmfInt32 Dest, XdmfInt64 Address, XdmfInt64 Length){
    XdmfDsmCommand  Cmd;
    XdmfInt32 Status;

    Cmd.Opcode = Opcode;
    Cmd.Source = this->Comm->GetId();
    Cmd.Target = Dest;
    Cmd.Address = Address;
    Cmd.Length = Length;

    this->Msg->SetSource(this->Comm->GetId());
    this->Msg->SetDest(Dest);
    this->Msg->SetLength(sizeof(Cmd));
    this->Msg->SetData(&Cmd);

    Status = this->Comm->Send(this->Msg);
    return(Status);
}

XdmfInt32
XdmfDsm::ReceiveCommandHeader(XdmfInt32 *Opcode, XdmfInt32 *Source, XdmfInt64 *Address, XdmfInt64 *Length, XdmfInt32 Block){
    XdmfDsmCommand  Cmd;
    XdmfInt32       status = XDMF_FAIL;

    this->Msg->Source = XDMF_DSM_ANY_SOURCE;
    this->Msg->SetLength(sizeof(Cmd));
    this->Msg->SetData(&Cmd);

    memset(&Cmd, 0, sizeof(XdmfDsmCommand));
    status = this->Comm->Check(this->Msg);
    if((status != XDMF_FAIL) || Block){
        status  = this->Comm->Receive(this->Msg);
        if(status == XDMF_FAIL){
            XdmfErrorMessage("Communicator Receive Failed");
            return(XDMF_FAIL);
        }else{
            *Opcode = Cmd.Opcode;
            *Source = Cmd.Source;
            *Address = Cmd.Address;
            *Length = Cmd.Length;
            status = XDMF_SUCCESS;
        }
    }
    return(status);
}

XdmfInt32
XdmfDsm::SendData(XdmfInt32 Dest, void *Data, XdmfInt64 Length){

    this->Msg->SetSource(this->Comm->GetId());
    this->Msg->SetDest(Dest);
    this->Msg->SetLength(Length);
    this->Msg->SetData(Data);
    return(this->Comm->Send(this->Msg));
}

XdmfInt32
XdmfDsm::ReceiveData(XdmfInt32 Source, void *Data, XdmfInt64 Length, XdmfInt32 Block){
    XdmfInt32   Status = XDMF_FAIL;

    this->Msg->SetSource(Source);
    this->Msg->SetLength(Length);
    this->Msg->SetData(Data);
    if(Block){
        Status = this->Comm->Receive(this->Msg);
    }else{
        Status = this->Comm->Check(this->Msg);
        if(Status == XDMF_SUCCESS){
            Status = this->Comm->Receive(this->Msg);
        }
    }
    return(Status);
}
