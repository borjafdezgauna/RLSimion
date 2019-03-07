/*
	SimionZoo: A framework for online model-free Reinforcement Learning on continuous
	control problems

	Copyright (c) 2016 SimionSoft. https://github.com/simionsoft

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

using System;
using System.Collections.Generic;
using System.Net;
using Caliburn.Micro;

using Herd.Network;
using Herd.Files;

namespace Badger.ViewModels
{
    public class HerdAgentViewModel : PropertyChangedBase
    {
        public HerdAgentInfo Info;
        bool m_bLocalAgent = false;

        public HerdAgentViewModel(HerdAgentInfo info)
        {
            Info = info;
            IpAddress = Info.ipAddress;

            //deselect local agent by default
            m_bLocalAgent = IsLocalIpAddress(IpAddressString);
            IsSelected = !m_bLocalAgent;
        }

        /// <summary>
        /// Determines whether an IP address is local
        /// </summary>
        /// <param name="host">The IP address</param>
        public static bool IsLocalIpAddress(string host)
        {
            try
            { // get host IP addresses
                IPAddress[] hostIPs = Dns.GetHostAddresses(host);
                // get local IP addresses
                IPAddress[] localIPs = Dns.GetHostAddresses(Dns.GetHostName());

                // test if any host IP equals to any local IP or to localhost
                foreach (IPAddress hostIP in hostIPs)
                {
                    // is localhost
                    if (IPAddress.IsLoopback(hostIP)) return true;
                    // is local address
                    foreach (IPAddress localIP in localIPs)
                    {
                        if (hostIP.Equals(localIP)) return true;
                    }
                }
            }
            catch { }
            return false;
        }

        private bool m_isSelected = false;
        public bool IsSelected
        {
            get { return m_isSelected; }
            set { m_isSelected = value; NotifyOfPropertyChange(() => IsSelected); }
        }

        //Interfaces to HerdAgentInfo object's properties. This avoids references to Caliburn from the Herd

        private IPEndPoint m_ipAddress;
        public IPEndPoint IpAddress
        {
            get { return m_ipAddress; }
            set
            {
                m_ipAddress = value;
                NotifyOfPropertyChange(() => IpAddress);
                NotifyOfPropertyChange(() => IpAddressString);
            }
        }

        private String m_authenticationCode = "";
        public String AuthenticationCode
        {
            get { return m_authenticationCode; }
            set
            {
                m_authenticationCode = value;
                NotifyOfPropertyChange(() => AuthenticationCode);

            }
        }

        public string IpAddressString { get { return IpAddress.Address.ToString(); } set { } }

        public DateTime lastACK
        {
            get { return Info.lastACK; }
            set { Info.lastACK = value; }
        }

        public string ProcessorId { get { return Info.ProcessorId; } }

        public int NumProcessors { get { if (!m_bLocalAgent) return Info.NumProcessors; else return Info.NumProcessors - 1; } }

        public string ProcessorArchitecture { get { return Info.ProcessorArchitecture; } }

        public string FormatedProcessorInfo
        {
            get
            {
                int cpus = Info.NumProcessors;
                string info = Info.ProcessorArchitecture + ", ";
                info += cpus + " Core";
                if (cpus > 1) info += 's';
                return info;
            }
        }

        public string ProcessorLoad
        {
            get { return Info.ProcessorLoad; }
        }

        public string Memory
        {
            get { return Info.Memory; }
        }

        public bool IsAvailable { get { return Info.IsAvailable; } }

        public string Version { get { return Info.Version; } }

        public string State
        {
            get { return Info.State; }
            set
            {
                Info.State = value;
                NotifyOfPropertyChange(() => State);
            }
        }

        public string CUDA
        {
            get
            {
                if (!Info.CUDA.Equals(PropValues.None))
                    return Info.CUDA;

                return "Not supported";
            }
        }

        public string CUDAColor
        {
            get
            {
                if (!Info.CUDA.Equals(PropValues.None))
                    return "Black";

                return "Tomato";
            }
        }

        static public AppVersion BestMatch(List<AppVersion> appVersions, HerdAgentViewModel agent)
        {
            foreach(AppVersion version in appVersions)
            {
                if (version.Requirements.Architecture == agent.ProcessorArchitecture)
                    return version;
            }
            return null;
        }
    }
}
