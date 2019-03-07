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
using System.Text.RegularExpressions;

using Herd.Files;

namespace SimionSrcParser
{
    public abstract class ChoiceElementParser
    {
        public abstract void parse(string content, ChoiceParameter parent);
    }
    public class ChoiceElementParserV1: ChoiceElementParser
    {
        public override void parse(string content, ChoiceParameter parent)
        {
          //  {
          //      { "Deterministic-Policy-Gaussian-Noise",CHOICE_ELEMENT_NEW<CDeterministicPolicyGaussianNoise>},
		        //{ "Stochastic-Policy-Gaussian-Noise",CHOICE_ELEMENT_NEW<CStochasticPolicyGaussianNoise>}
          //  });
            string sChoiceElementPattern = @"{\s*""([^""]+)"",\s*CHOICE_ELEMENT_(NEW|FACTORY)\s*<(\w+)>\s*}";
            foreach (Match choiceElementMatch in Regex.Matches(content, sChoiceElementPattern))
            {
                string choiceElementName = choiceElementMatch.Groups[1].Value.Trim(' ', '"');
                string choiceElementClass = choiceElementMatch.Groups[3].Value.Trim(' ', '"');
                string choiceElementType = choiceElementMatch.Groups[2].Value.Trim(' ', '"');
                ChoiceElementParameter.Type type;
                if (choiceElementType == "NEW") type = ChoiceElementParameter.Type.New;
                else type = ChoiceElementParameter.Type.Factory;

                ChoiceElementParameter choiceElement = new ChoiceElementParameter(choiceElementName
                    , choiceElementClass, type);
                parent.AddParameter(choiceElement);
            }
        }
    }
    public class ChoiceElementParserV2: ChoiceElementParser
    {
        public override void parse(string content, ChoiceParameter parent)
        {
          //  {
          //      { make_tuple("Wind-turbine", CHOICE_ELEMENT_NEW<CWindTurbine>, "World=Wind-turbine")},
		        //{ make_tuple("Underwater-vehicle", CHOICE_ELEMENT_NEW<CUnderwaterVehicle>, "World=Underwater-vehicle")},
		        //{ make_tuple("Pitch-control", CHOICE_ELEMENT_NEW<CPitchControl>, "World=Pitch-control")},
		        //{ make_tuple("Balancing-pole", CHOICE_ELEMENT_NEW<CBalancingPole>, "World=Balancing-pole")},
		        //{ make_tuple("Mountain-car", CHOICE_ELEMENT_NEW<CMountainCar>, "World=Mountain-car") }
          //  });
            string sChoiceElementPattern = @"{\s*make_tuple\s*\(\s*""([^""]+)""\s*,\s*CHOICE_ELEMENT_(NEW|FACTORY)\s*<(\w+)>\s*,\s*""([^""]*)""\s*\)\s*}";
            foreach (Match choiceElementMatch in Regex.Matches(content, sChoiceElementPattern))
            {
                string choiceElementName = choiceElementMatch.Groups[1].Value.Trim(' ', '"');
                string choiceElementClass = choiceElementMatch.Groups[3].Value.Trim(' ', '"');
                string choiceElementType = choiceElementMatch.Groups[2].Value.Trim(' ', '"');
                string badgerMetadata = choiceElementMatch.Groups[4].Value.Trim(' ', '"');
                ChoiceElementParameter.Type type;
                if (choiceElementType == "NEW") type = ChoiceElementParameter.Type.New;
                else type = ChoiceElementParameter.Type.Factory;

                ChoiceElementParameter choiceElement = new ChoiceElementParameter(choiceElementName
                    , choiceElementClass, type, badgerMetadata);
                parent.AddParameter(choiceElement);
            }
        }
    }
    public class ChoiceParser
    {
        private List<ChoiceElementParser> m_choiceElementParsers= new List<ChoiceElementParser>();
        protected List<IParameter> choices = new List<IParameter>();
        public ChoiceParser()
        {
            m_choiceElementParsers.Add(new ChoiceElementParserV1());
            m_choiceElementParsers.Add(new ChoiceElementParserV2());
        }
        public void parse(ParameterizedObject parent, string srcCode)
        {
            string sPattern = @"CHOICE\s*<\s*(\w+)\s*>\(([^,]+),([^,]+),([^,]+),([^;]*);";

            foreach (Match match in Regex.Matches(srcCode, sPattern))
            {
                string className = match.Groups[1].Value.Trim(' ', '"');
                string choiceName = match.Groups[3].Value.Trim(' ', '"');
                string comment = match.Groups[4].Value.Trim(' ', '"');
                string choiceElements,prefix;
                choiceElements = match.Groups[5].Value.Trim(' ', '"', '\t', '\n');
                CppSourceParser.GetEnclosedBody(match.Groups[5].Value.Trim(' ', '"','\t','\n'),0,"{","}",out choiceElements,out prefix);
                choiceElements= choiceElements.Trim(' ', '"', '\t', '\n');
                ChoiceParameter newChoice = new ChoiceParameter(className,choiceName, comment);

                foreach (ChoiceElementParser choiceElementParser in m_choiceElementParsers)
                    choiceElementParser.parse(choiceElements, newChoice);
                
                parent.addParameter(newChoice);
            }
        }
    }
 

    public class Factory: ParameterizedObject
    {
        private ChoiceParser m_choiceParser= new ChoiceParser();

        public Factory(string className, string paramName, string body, string bodyPrefix)
        {
            m_name = className;
            m_choiceParser.parse(this, body);
        }

        public override string OutputXML(int level)
        {
            string output = "";
            FileFormatter.AddIndentation(ref output, level);
            output += "<" + XMLTags.ClassDefinitionNodeTag + " Name=\"" + m_name + "\">\n";
            output += OutputChildrenXML(level + 1);
            FileFormatter.AddIndentation(ref output, level);
            output += "</" + XMLTags.ClassDefinitionNodeTag + ">\n";
            return output;
        }
    }
}
