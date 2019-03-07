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

using Herd.Files;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Xml.Linq;

namespace Herd.Network
{
    public class HerdAgentInfo
    {
        private IPEndPoint m_ipAddress;
        public IPEndPoint ipAddress { get { return m_ipAddress; } set { m_ipAddress = value; } }

        public string IpAddressString
        {
            get
            {
                return m_ipAddress.Address.ToString();
            }
        }

        public DateTime lastACK { get; set; }

        private Dictionary<string, string> m_properties;

        public HerdAgentInfo()
        {
            CultureInfo customCulture = (CultureInfo)Thread.CurrentThread.CurrentCulture.Clone();
            customCulture.NumberFormat.NumberDecimalSeparator = ".";
            Thread.CurrentThread.CurrentCulture = customCulture;

            m_properties = new Dictionary<string, string>();
        }

        /// <summary>
        /// Constructor used for testing purposes
        /// </summary>
        public HerdAgentInfo(string processorId, int numCPUCores, string architecture, string CUDAVersion, string herdAgentVersion)
        {
            CultureInfo customCulture = (CultureInfo)Thread.CurrentThread.CurrentCulture.Clone();
            customCulture.NumberFormat.NumberDecimalSeparator = ".";
            Thread.CurrentThread.CurrentCulture = customCulture;

            m_ipAddress = new IPEndPoint(0, 0);
            m_properties = new Dictionary<string, string>
            {
                [PropNames.ProcessorId] = processorId,
                [PropNames.NumCPUCores] = numCPUCores.ToString(),
                [PropNames.Architecture] = architecture,
                [PropNames.CUDA] = CUDAVersion,
                [PropNames.HerdAgentVersion] = herdAgentVersion
            };
        }

        /// <summary>
        /// Adds a property to the agent description
        /// </summary>
        /// <param name="name">The name of the property</param>
        /// <param name="value">The value of the property</param>
        public void AddProperty(string name, string value)
        {
            if (!m_properties.ContainsKey(name))
                m_properties.Add(name, value);
            else m_properties[name] = value;
        }

        /// <summary>
        /// Gets the value of the specified property
        /// </summary>
        /// <param name="name">The name of the property</param>
        /// <returns>Its value</returns>
        public string Property(string name)
        {
            if (m_properties.ContainsKey(name))
                return m_properties[name];

            return PropValues.None;
        }

        /// <summary>
        /// Parses the specified XML herd agent description filling the properties of the herd agent
        /// </summary>
        /// <param name="xmlDescription">The XML description.</param>
        public void Parse(XElement xmlDescription)
        {
            if (xmlDescription.Name.ToString() == XmlTags.HerdAgentDescription)
            {
                m_properties.Clear();

                foreach (XElement child in xmlDescription.Elements())
                    AddProperty(child.Name.ToString(), child.Value);
            }
        }

        /// <summary>
        /// Converts the list of properties to a string
        /// </summary>
        public override string ToString()
        {
            string res = "";
            foreach (var property in m_properties)
                res += property.Key + "=\"" + property.Value + "\";";

            return res;
        }


        public string State { get { return Property(PropNames.State); } set { } }

        public string Version { get { return Property(PropNames.HerdAgentVersion); } set { } }

        public string ProcessorId { get { return Property(PropNames.ProcessorId); } }

        public int NumProcessors
        {
            get
            {
                string prop = Property(PropNames.NumCPUCores);
                if (prop == PropValues.None)
                    prop = Property(Deprecated.NumCPUCores); //for retrocompatibility
                return (!prop.Equals(PropValues.None)) ? int.Parse(prop) : 0;
            }
        }

        public string ProcessorArchitecture
        {
            get
            {
                string prop = Property(PropNames.Architecture);
                if (prop == PropValues.None)
                {
                    prop = Property(Deprecated.ProcessorArchitecture); //for retrocompatibility
                    if (prop == "AMD64" || prop == "IA64")
                        return PropValues.Win64;
                    else
                        return PropValues.Win32;
                }
                else
                    return prop;
            }
        }

        public string ProcessorLoad
        {
            get
            {
                //The value reported by the herd agent might be a number with either a comma or dot delimiter and
                //any number of decimal values: 3.723242 or 3,723242
                //We normalize the format with a dot, only two decimal values and the percent symbol 3.72%
                string processorLoad = Property(PropNames.ProcessorLoad);
                processorLoad = processorLoad.Replace(',', '.');
                int delimiterPos = processorLoad.LastIndexOf('.');
                if (delimiterPos > 0)
                    processorLoad = processorLoad.Substring(0, Math.Min(processorLoad.Length, delimiterPos + 3)) + "%";
                else processorLoad = processorLoad + "%";
                return processorLoad;
            }
        }

        public string Memory
        {
            get
            {
                //The value reported by the herd agent might be the absolute number of bytes or the number of megabytes, gigabytes....
                //For example: 12341234, 12341234Mb, 12341234Gb
                //We normalize the format in Gigabytes and using one decimal values AT MOST: 1.2Gb, 0.5Gb, 1204Gb
                //We assume the minimum available memory will be 512Mb
                string totalMemory = Property(PropNames.TotalMemory);
                double multiplier = 1;

                string ending = totalMemory.Substring(totalMemory.Length - 2, 2);

                if (ending == "Gb")
                    return totalMemory; // no tranformation needed?
                else if (ending == "Mb")
                {
                    multiplier = 1.0 / 1024;
                    totalMemory = totalMemory.Substring(0, totalMemory.Length - 2);
                }
                else if (ending == "Kb")
                {
                    multiplier = 1.0 / (1024 * 1024);
                    totalMemory = totalMemory.Substring(0, totalMemory.Length - 2);
                }
                else
                {
                    multiplier = 1.0 / (1024 * 1024 * 1024);
                }

                double memoryInGbs;
                double.TryParse(totalMemory, out memoryInGbs);
                memoryInGbs *= multiplier;

                //Remove unnecessary decimals
                memoryInGbs = Math.Round(memoryInGbs, 1);
                totalMemory = memoryInGbs.ToString(CultureInfo.GetCultureInfo("en-US"));

                return memoryInGbs.ToString() + "Gb";
            }
        }

        public string CUDA
        {
            get
            {
                string prop = Property(PropNames.CUDA);
                if (prop == PropValues.None)
                    prop = Property(Deprecated.CUDA);  //for retrocompatibility
                return prop;
            }
        }

        public bool IsAvailable
        {
            get { if (Property(PropNames.State) == PropValues.StateAvailable) return true; return false; }
            set { }
        }

        /// <summary>
        /// Returns the app version of an experimental unit that best matches the herd agent
        /// </summary>
        /// <param name="experimentalUnit">The experimental unit</param>
        /// <returns>The best matching app version</returns>
        public AppVersion BestMatch(ExperimentalUnit experimentalUnit)
        {
            foreach (AppVersion version in experimentalUnit.AppVersions)
            {
                if (version.Requirements.Architecture == ProcessorArchitecture
                    && experimentalUnit.RunTimeReqs.CanBeRunOnArchitecture(ProcessorArchitecture))
                    return version;
            }
            return null;
        }
    }
      
}
