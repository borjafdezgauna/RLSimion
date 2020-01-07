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

using System.ComponentModel;
using Caliburn.Micro;
using OxyPlot;
using System.Runtime.Serialization;

namespace Badger.ViewModels
{
    [DataContract]
    public class PlotPropertiesViewModel : PropertyChangedBase
    {
        /*
         * plot properties:
         * x in/out
         * show legend
         * 
         * line series properties:
         * -visible
         * -[color, thickness,...]
         */

        //all-track selection 
        private bool m_bAllTracksSelected = true;
        [DataMember]
        public bool AllTracksSelected
        {
            get { return m_bAllTracksSelected; }
            set
            {
                m_bAllTracksSelected = value;
                NotifyOfPropertyChange(() => AllTracksSelected);

                //we must check this because when loaded from a file, might not yet have been read line properties
                if (LineSeriesProperties != null)
                {
                    foreach (PlotLineSeriesPropertiesViewModel trackProperties in LineSeriesProperties)
                        trackProperties.Visible = value;
                }
            }
        }
        //per-track series properties
        [DataMember]
        public BindableCollection<PlotLineSeriesPropertiesViewModel> LineSeriesProperties { get; set; } = new BindableCollection<PlotLineSeriesPropertiesViewModel>();

        public void AddLineSeries(string name, string description, OxyPlot.Series.LineSeries series)
        {
            PlotLineSeriesPropertiesViewModel newLineProperties 
                = new PlotLineSeriesPropertiesViewModel(name, description, series);
            LineSeriesProperties.Add(newLineProperties);
            newLineProperties.PropertyChanged += RaisePropertiesChangedEvent;
        }

        public PlotLineSeriesPropertiesViewModel LineSeriesByName(string name)
        {
            foreach (PlotLineSeriesPropertiesViewModel lineSeriesProperties in LineSeriesProperties)
                if (lineSeriesProperties.Name == name)
                    return lineSeriesProperties;
            return null;
        }

        public bool PropertiesChanged { get; set; }
        public void RaisePropertiesChangedEvent(object sender, PropertyChangedEventArgs e)
        {
            NotifyOfPropertyChange(() => PropertiesChanged);
        }

        //LEGEND
        //Position
        [DataMember]
        public BindableCollection<string> LegendPositions { get; set; }= new BindableCollection<string>();
        private string m_selectedLegendPosition;
        [DataMember]
        public string SelectedLegendPosition
        {
            get { return m_selectedLegendPosition; }
            set { m_selectedLegendPosition = value; NotifyOfPropertyChange(() => SelectedLegendPosition); }
        }
        //Placement
        [DataMember]
        public BindableCollection<string> LegendPlacements { get; set; }= new BindableCollection<string>();

        private string m_selectedLegendPlacement;
        [DataMember]
        public string SelectedLegendPlacement
        {
            get { return m_selectedLegendPlacement; }
            set { m_selectedLegendPlacement = value; NotifyOfPropertyChange(() => SelectedLegendPlacement); }
        }
        //Orientation
        [DataMember]
        public BindableCollection<string> LegendOrientations { get; set; }= new BindableCollection<string>();

        private string m_selectedLegendOrientation;
        [DataMember]
        public string SelectedLegendOrientation
        {
            get { return m_selectedLegendOrientation; }
            set { m_selectedLegendOrientation = value; NotifyOfPropertyChange(() => SelectedLegendOrientation); }
        }
        //Visibility
        private bool m_bLegendVisible = true;
        [DataMember]
        public bool LegendVisible
        {
            get { return m_bLegendVisible; }
            set { m_bLegendVisible = value; NotifyOfPropertyChange(() => LegendVisible); }
        }
        private bool m_bLegendBorder = true;
        [DataMember]
        public bool LegendBorder
        {
            get { return m_bLegendBorder; }
            set { m_bLegendBorder = value; NotifyOfPropertyChange(() => LegendBorder); }
        }
        private bool m_bLegendSolidBackground = true;
        [DataMember]
        public bool LegendSolidBackground
        {
            get { return m_bLegendSolidBackground; }
            set { m_bLegendSolidBackground = value; NotifyOfPropertyChange(() => LegendSolidBackground); }
        }
        //Font
        [DataMember]
        public BindableCollection<string> Fonts { get; set; } = new BindableCollection<string>();

        private string m_selectedFont;
        [DataMember]
        public string SelectedFont
        {
            get { return m_selectedFont; }
            set { m_selectedFont = value;NotifyOfPropertyChange(() => SelectedFont); }
        }
        //Texts
        private string m_title = "N/A";
        [DataMember]
        public string PlotTitle
        {
            get { return m_title; }
            set { m_title = value; NotifyOfPropertyChange(() => PlotTitle); }
        }
        private string m_xAxisName = "N/A";
        [DataMember]
        public string XAxisName
        {
            get { return m_xAxisName; }
            set { m_xAxisName = value; NotifyOfPropertyChange(() => XAxisName); }
        }
        private string m_yAxisName = "N/A";
        [DataMember]
        public string YAxisName
        {
            get { return m_yAxisName; }
            set { m_yAxisName = value; NotifyOfPropertyChange(() => YAxisName); }
        }

        public PlotPropertiesViewModel()
        {
            LegendPositions.Add(LegendPosition.BottomLeft.ToString());
            LegendPositions.Add(LegendPosition.BottomRight.ToString());
            LegendPositions.Add(LegendPosition.TopLeft.ToString());
            LegendPositions.Add(LegendPosition.TopRight.ToString());
            SelectedLegendPosition = LegendPosition.TopRight.ToString();

            LegendPlacements.Add(LegendPlacement.Inside.ToString());
            LegendPlacements.Add(LegendPlacement.Outside.ToString());
            SelectedLegendPlacement = LegendPlacement.Inside.ToString();

            LegendOrientations.Add(LegendOrientation.Horizontal.ToString());
            LegendOrientations.Add(LegendOrientation.Vertical.ToString());
            SelectedLegendOrientation = LegendOrientation.Vertical.ToString();

            Fonts.Add("Arial");
            Fonts.Add("Segoe UI");
            Fonts.Add("Times New Roman");
            Fonts.Add("Verdana");
            Fonts.Add("CMU Serif");
            Fonts.Add("CMU Sans Serif");
            SelectedFont = "Segoe UI";
        }
        /// <summary>
        ///     Highlight a series 
        /// </summary>
        /// <param name="seriesId"></param>
        public void HighlightSeries(int seriesId)
        {
            for (int i= 0; i<LineSeriesProperties.Count; i++)
            {
                if (i == seriesId)
                    ResetLineSeriesOpacity(LineSeriesProperties[i]);
                else DimLineSeriesColor(LineSeriesProperties[i]);
            }
        }
        /// <summary>
        ///     Apply some opacity to the original color of the LineSeries.  
        /// </summary>
        /// <param name="lineSeriesProperties"></param>
        public void DimLineSeriesColor(PlotLineSeriesPropertiesViewModel lineSeriesProperties)
        {
            OxyColor color = lineSeriesProperties.LineSeries.ActualColor;
            // Apply an opacity of 15% over the original color
            lineSeriesProperties.LineSeries.Color = OxyColor.FromArgb(0x26, color.R, color.G, color.B);
        }

        /// <summary>
        ///     Restore the original color of the LineSeries.  
        /// </summary>
        /// <param name="lineSeriesProperties"></param>
        public void ResetLineSeriesOpacity(PlotLineSeriesPropertiesViewModel lineSeriesProperties)
        {
            OxyColor color = lineSeriesProperties.LineSeries.ActualColor;
            lineSeriesProperties.LineSeries.Color = OxyColor.FromRgb(color.R, color.G, color.B);
        }

        public void SetNotifying(bool notifying)
        {
            IsNotifying = notifying;
            foreach(PlotLineSeriesPropertiesViewModel properties in LineSeriesProperties)
            {
                properties.IsNotifying = notifying;
                properties.PropertyChanged += RaisePropertiesChangedEvent;
            }
        }
    }
}
