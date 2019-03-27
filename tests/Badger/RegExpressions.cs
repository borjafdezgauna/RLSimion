﻿using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Text.RegularExpressions;
using Herd.Network;

namespace RegExpressions
{
    [TestClass]
    public class UnitTest1
    {
        [TestMethod]
        public void Badger_RegEx_TaskHeader()
        {
            string testTaskHeader= "<Task Name=\"TaskName\" Exe=\"../RLSimion.exe\" Arguments=\"../RLSimion.exe -pipe=harl\" Pipe=\"harl\" AuthenticationToken=\"token\"/>";
            Match match = Regex.Match(testTaskHeader, JobTransmitter.TaskHeaderRegEx);
            Assert.AreEqual(7, match.Groups.Count);
            Assert.AreEqual("TaskName", match.Groups[1].Value);
            Assert.AreEqual("../RLSimion.exe", match.Groups[2].Value);
            Assert.AreEqual("../RLSimion.exe -pipe=harl", match.Groups[3].Value);
            Assert.AreEqual("harl", match.Groups[4].Value);
            Assert.AreEqual(" AuthenticationToken=\"token\"", match.Groups[5].Value);
            Assert.AreEqual("token", match.Groups[6].Value);

            testTaskHeader = "<Task Name=\"TaskName\" Exe=\"../RLSimion.exe\" Arguments=\"../RLSimion.exe -pipe=harl\" Pipe=\"harl\"/>";
            match = Regex.Match(testTaskHeader, JobTransmitter.TaskHeaderRegEx);
            Assert.AreEqual(7, match.Groups.Count);
            Assert.AreEqual("TaskName", match.Groups[1].Value);
            Assert.AreEqual("../RLSimion.exe", match.Groups[2].Value);
            Assert.AreEqual("../RLSimion.exe -pipe=harl", match.Groups[3].Value);
            Assert.AreEqual("harl", match.Groups[4].Value);
            Assert.AreEqual("", match.Groups[5].Value);
            Assert.AreEqual("", match.Groups[6].Value);

            //Let's check that old agents will be able to parse tasks without auth. tokens
            testTaskHeader = "<Task Name=\"TaskName\" Exe=\"../RLSimion.exe\" Arguments=\"../RLSimion.exe -pipe=harl\" Pipe=\"harl\"/>";
            match = Regex.Match(testTaskHeader, JobTransmitter.LegacyTaskHeaderRegEx);
            Assert.AreEqual(5, match.Groups.Count);
            Assert.AreEqual("TaskName", match.Groups[1].Value);
            Assert.AreEqual("../RLSimion.exe", match.Groups[2].Value);
            Assert.AreEqual("../RLSimion.exe -pipe=harl", match.Groups[3].Value);
            Assert.AreEqual("harl", match.Groups[4].Value);

        }
    }
}
