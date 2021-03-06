/**
Software License Agreement (BSD)
\file      virtual_stigmergy.h 
\authors Xuefeng Chang <changxuefengcn@163.com>
\copyright Copyright (c) 2016, the micROS Team, HPCL (National University of Defense Technology), All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the
   following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
   following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of micROS Team, HPCL, nor the names of its contributors may be used to endorse or promote
   products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WAR-
RANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, IN-
DIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef VIRTUAL_STIGMERGY_H_
#define VIRTUAL_STIGMERGY_H_

#include <iostream>
#include <string>
#include <time.h>
#include <stdlib.h>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <queue>
#include <algorithm>

#include "ros/ros.h"

#include "micros_swarm_framework/message.h"
#include "micros_swarm_framework/singleton.h"
#include "micros_swarm_framework/runtime_platform.h"
#include "micros_swarm_framework/communication_interface.h"
#ifdef ROS
#include "micros_swarm_framework/ros_communication.h"
#endif
#ifdef OPENSPLICE_DDS
#include "micros_swarm_framework/opensplice_dds_communication.h"
#endif

namespace micros_swarm_framework{
    
    template<class Type>
    class VirtualStigmergy{
        public:
            VirtualStigmergy(){vstig_id_=-1;}
            
            VirtualStigmergy(int vstig_id)
            {
                vstig_id_=vstig_id;
                rtp_=Singleton<RuntimePlatform>::getSingleton();
                #ifdef ROS
                communicator_=Singleton<ROSCommunication>::getSingleton();
                #endif
                #ifdef OPENSPLICE_DDS
                communicator_=Singleton<OpenSpliceDDSCommunication>::getSingleton();
                #endif
                rtp_->createVirtualStigmergy(vstig_id_);
            }
            
            VirtualStigmergy(const VirtualStigmergy& vs)
            {
                rtp_=Singleton<RuntimePlatform>::getSingleton();
                #ifdef ROS
                communicator_=Singleton<ROSCommunication>::getSingleton();
                #endif
                #ifdef OPENSPLICE_DDS
                communicator_=Singleton<OpenSpliceDDSCommunication>::getSingleton();
                #endif
                vstig_id_=vs.vstig_id_;
            }
            
            VirtualStigmergy& operator=(const VirtualStigmergy& vs)
            {
                if(this==&vs)
                    return *this;
                rtp_=Singleton<RuntimePlatform>::getSingleton();
                #ifdef ROS
                communicator_=Singleton<ROSCommunication>::getSingleton();
                #endif
                #ifdef OPENSPLICE_DDS
                communicator_=Singleton<OpenSpliceDDSCommunication>::getSingleton();
                #endif
                vstig_id_=vs.vstig_id_;
                return *this;
            }
            
            ~VirtualStigmergy(){}
            
            void put(const std::string& key, const Type& data)
            {
                std::ostringstream archiveStream;
                boost::archive::text_oarchive archive(archiveStream);
                archive<<data;
                std::string s=archiveStream.str();
                
                rtp_->insertOrUpdateVirtualStigmergy(vstig_id_, key, s, time(0), rtp_->getRobotID());
                
                VirtualStigmergyPut vsp(vstig_id_, key, s, time(0), rtp_->getRobotID());
                
                std::ostringstream archiveStream2;
                boost::archive::text_oarchive archive2(archiveStream2);
                archive2<<vsp;
                std::string vsp_str=archiveStream2.str();   
                      
                micros_swarm_framework::MSFPPacket p;
                p.packet_source=rtp_->getRobotID();
                p.packet_version=1;
                p.packet_type=VIRTUAL_STIGMERGY_PUT;
                #ifdef ROS
                p.packet_data=vsp_str;
                #endif
                #ifdef OPENSPLICE_DDS
                //std::cout<<"vsp_str.data(): "<<vsp_str.data()<<std::endl;
                p.packet_data=vsp_str.data();
                #endif
                p.package_check_sum=0;
                
                rtp_->getOutMsgQueue()->pushVstigMsgQueue(p);
            }
            
            Type get(const std::string& key)
            {
                VirtualStigmergyTuple vst;
                rtp_->getVirtualStigmergyTuple(vstig_id_, key, vst);
                
                if(vst.vstig_timestamp==0)
                {
                    std::cout<<"ID "<<vstig_id_<<" virtual stigmergy, "<<key<<" is not exist."<<std::endl;
                    exit(-1);
                }
                
                std::string data_str=vst.vstig_value;
                Type data;
                std::istringstream archiveStream(data_str);
                boost::archive::text_iarchive archive(archiveStream);
                archive>>data;
                
                VirtualStigmergyQuery vsq(vstig_id_, key, vst.vstig_value, vst.vstig_timestamp, vst.robot_id);
                
                std::ostringstream archiveStream2;
                boost::archive::text_oarchive archive2(archiveStream2);
                archive2<<vsq;
                std::string vsg_str=archiveStream2.str();  
                
                micros_swarm_framework::MSFPPacket p;
                p.packet_source=rtp_->getRobotID();
                p.packet_version=1;
                p.packet_type=VIRTUAL_STIGMERGY_QUERY;
                #ifdef ROS
                p.packet_data=vsg_str;
                #endif
                #ifdef OPENSPLICE_DDS
                //std::cout<<"vsg_str.data(): "<<vsg_str.data()<<std::endl;
                p.packet_data=vsg_str.data();
                #endif
                p.package_check_sum=0;
                
                rtp_->getOutMsgQueue()->pushVstigMsgQueue(p);
                
                return data;  
            }
            
            int size()
            {
                return rtp_->getVirtualStigmergySize(vstig_id_);
            }
        private:
            int vstig_id_;
            boost::shared_ptr<RuntimePlatform> rtp_;
            boost::shared_ptr<CommunicationInterface> communicator_;
    };
}
#endif
