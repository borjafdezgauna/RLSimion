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
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimionSrcParser
{
    public abstract class ParameterizedObject
    {
        protected string m_name;
        public string name { get { return m_name; } }
        private List<IParameter> m_parameters = new List<IParameter>();
        public List<IParameter> Parameters { get { return m_parameters; } }
        public void addParameter(IParameter parameter)
        {
            parameter.SetParameterIndexInCode(m_parameters.Count);
            m_parameters.Add(parameter);
        }

        public abstract string OutputXML(int level);
        public string OutputChildrenXML(int level)
        {
            string output = "";
            var orderedList = m_parameters.OrderBy(param => param.ParameterClassSortValue()).ThenBy(param=>param.GetName());
            foreach (IParameter parameter in orderedList)
                output += parameter.AsXML(level);
            return output;
        }
        public virtual bool IsWorld() { return false; }
    }
}
