import React, { Component, useLayoutEffect, useRef } from 'react';

import chroma from 'chroma-js';

import * as am4core from "@amcharts/amcharts4/core";
import * as am4charts from "@amcharts/amcharts4/charts";
import am4themes_animated from "@amcharts/amcharts4/themes/animated";

am4core.useTheme(am4themes_animated);

// https://www.amcharts.com/docs/v4/tutorials/creating-themes/
function am4themes_myTheme(target) {
  if (target instanceof am4core.InterfaceColorSet) {
    target.setFor("background", am4core.color("#000000"));
    target.setFor("fill", am4core.color("#000000"));
    target.setFor("grid", am4core.color("#aaaaaa"));
    target.setFor("text", am4core.color("#eeeeee"));
  }
}
am4core.useTheme(am4themes_myTheme);

const newBullet = () => {
  const c = new am4charts.CircleBullet();
  c.circle.radius = 2;
  c.circle.strokeOpacity = 0;
  return c;
}

let colorSet = new am4core.ColorSet();

const colorTemperature2rgb = function(kelvin) {

  var temperature = kelvin / 100.0;
  var red, green, blue;

  if (temperature < 66.0) {
    red = 255;
  } else {
    // a + b x + c Log[x] /.
    // {a -> 351.97690566805693`,
    // b -> 0.114206453784165`,
    // c -> -40.25366309332127
    //x -> (kelvin/100) - 55}
    red = temperature - 55.0;
    red = 351.97690566805693+ 0.114206453784165 * red - 40.25366309332127 * Math.log(red);
    if (red < 0) red = 0;
    if (red > 255) red = 255;
  }

  /* Calculate green */

  if (temperature < 66.0) {

    // a + b x + c Log[x] /.
    // {a -> -155.25485562709179`,
    // b -> -0.44596950469579133`,
    // c -> 104.49216199393888`,
    // x -> (kelvin/100) - 2}
    green = temperature - 2;
    green = -155.25485562709179 - 0.44596950469579133 * green + 104.49216199393888 * Math.log(green);
    if (green < 0) green = 0;
    if (green > 255) green = 255;

  } else {

    // a + b x + c Log[x] /.
    // {a -> 325.4494125711974`,
    // b -> 0.07943456536662342`,
    // c -> -28.0852963507957`,
    // x -> (kelvin/100) - 50}
    green = temperature - 50.0;
    green = 325.4494125711974 + 0.07943456536662342 * green - 28.0852963507957 * Math.log(green);
    if (green < 0) green = 0;
    if (green > 255) green = 255;

  }

  /* Calculate blue */

  if (temperature >= 66.0) {
    blue = 255;
  } else {

    if (temperature <= 20.0) {
      blue = 0;
    } else {

      // a + b x + c Log[x] /.
      // {a -> -254.76935184120902`,
      // b -> 0.8274096064007395`,
      // c -> 115.67994401066147`,
      // x -> kelvin/100 - 10}
      blue = temperature - 10;
      blue = -254.76935184120902 + 0.8274096064007395 * blue + 115.67994401066147 * Math.log(blue);
      if (blue < 0) blue = 0;
      if (blue > 255) blue = 255;
    }
  }

  return chroma(Math.round(red), Math.round(blue), Math.round(green));
}

class App extends Component {

componentDidMount() {
//  const chart = useRef(null);

/*  useEffect(() => {
    Papa.parse("/data.tsv", {
      download: true,
      dynamicTyping: true,
      complete: (results) => {
        setLogData([
          {
            id: 'lux',
            data: results.data.map((r) => Object.fromEntries([['x', new Date(r[0])], ['y', Math.max(.1, r[1])]]))
          },
        ]);
        setData([
          {
            id: 'cct',
            data: results.data.map((r) => Object.fromEntries([['x', new Date(r[0])], ['y', r[2]]]))
          },
          {
            id: 'v',
            data: results.data.map((r) => Object.fromEntries([['x', new Date(r[0])], ['y', r[3]]]))
          },
/*          {
            id: 'temp',
            data: results.data.map((r) => Object.fromEntries([['x', new Date(r[0])], ['y', r[4]]]))
          },
          {
            id: 'hum',
            data: results.data.map((r) => Object.fromEntries([['x', new Date(r[0])], ['y', r[5]]]))
          },
        ]);
      }
    });
  }, []);*/

// useLayoutEffect(() => {
    let x = am4core.create("chartdiv", am4charts.XYChart);
    x.dateFormatter = new am4core.DateFormatter();
    x.dateFormatter.timezoneOffset = 8*60;

    x.legend = new am4charts.Legend();
    x.zoomOutButton.valign = "bottom";
//    x.zoomOutButton.parent = x.tooltipContainer;

    var prevData = [];
    var appendDataIfAny;
    appendDataIfAny = (ev) => {
      button.disabled = ev.target.data.length === 0;
      if (button.disabled) {
        x.events.once("beforedatavalidated", appendDataIfAny);
        return;
      }
      let firstUploadAttempt = true;
      for (let i = ev.target.data.length - 1; i > 0; i--) {
        if (ev.target.data[i].fails) {
          ev.target.data[i].upload = firstUploadAttempt;
          firstUploadAttempt = ev.target.data[i].fails === 1;
          ev.target.data[i].vl = ev.target.data[i].v / 200;
          // rough heuristic:
//          ev.target.data[i].vp = 200 * (ev.target.data[i].vl - 3.7);
          // better: https://i.stack.imgur.com/LV91V.gif
          const thresholds = [3.4, 3.68, 3.74, 3.8, 3.95, 4.2, 1000]; // first one could also be 3.0 for completely dead
          const th = Math.max(1, thresholds.findIndex((el) => ev.target.data[i].vl < el));
          ev.target.data[i].vp = Math.min(100, Math.round(20 * (th - 1 + (ev.target.data[i].vl - thresholds[th-1]) / (thresholds[th] - thresholds[th-1]))));
        } else {
          ev.target.data[i].vv = ev.target.data[i].v / 200;
        }
      }
      console.log(prevData.length);
      console.log(ev.target.data.length);
      ev.target.data = ev.target.data.concat(prevData);
      console.log(ev.target.data.length);
      button.enabled = true;
//      button.interactionsEnabled = true;
    };

    const button = x.chartContainer.createChild(am4core.Button);
    button.disabled = true;
    button.label.text = "<< get more data";
    button.align = "left";
    button.marginRight = 15;
    button.events.on("hit", function() {
      prevData = x.data;
//      button.interactionsEnabled = false;
      button.enabled = false;
      x.events.once("beforedatavalidated", appendDataIfAny);
      x.dataSource.url = 'https://thiswasyouridea.com/thelightonmyroof/data.php?before=' + x.data[0].ts.getTime();
      x.dataSource.load();
    });

    x.events.once("beforedatavalidated", appendDataIfAny);

    // chart.legend.itemContainers.template.clickable = false;
    // chart.legend.itemContainers.template.focusable = false;
    // chart.legend.itemContainers.template.cursorOverStyle = am4core.MouseCursorStyle.default;
    x.paddingRight = 20;

    x.leftAxesContainer.layout = "vertical";

    let dateAxis = x.xAxes.push(new am4charts.DateAxis());
    dateAxis.renderer.grid.template.location = 0;
    // not working: https://www.amcharts.com/docs/v4/tutorials/pre-zooming-an-axis/
//    dateAxis.keepSelection = true;
//    dateAxis.start = 0.8;
//    dateAxis.end = 1;
    // TODO also fix scale? https://www.amcharts.com/docs/v4/tutorials/fixed-value-axis-scale/
    dateAxis.showOnInit = false;
    // ...
/*    x.events.on("ready", function () {
      dateAxis.zoomToDates(
        new Date(2021, 7, 4),
        new Date(2021, 7, 7),
        false,
        true // this makes zoom instant
      );
    });
*/
    dateAxis.groupData = true;
    // maximum number of data items we allow to be displayed at a time, default 200
    dateAxis.groupCount = 100;
    dateAxis.groupIntervals.setAll([
      { timeUnit: "second", count: 4 },
      { timeUnit: "second", count: 8 },
      { timeUnit: "second", count: 15 },
      { timeUnit: "second", count: 30 },
      { timeUnit: "minute", count: 1 },
      { timeUnit: "minute", count: 2 },
      { timeUnit: "minute", count: 4 },
      { timeUnit: "minute", count: 8 },
      { timeUnit: "minute", count: 15 },
      { timeUnit: "minute", count: 30 },
      { timeUnit: "hour", count: 1 },
    ]);

    const seriesToggleListener = (e) => {
      let disabled = true;
      e.target.yAxis.series.each(function(series) {
        if (!series.isHiding && !series.isHidden) {
          disabled = false;
        }
      });
      e.target.yAxis.disabled = disabled;
    };

    const addSeries = (name, column, hidden = false, unit = '', color = undefined, seriesType = new am4charts.LineSeries(), axis = true, tooltip = '{' + column + '}' + unit) => {
      if (axis === true) {
        axis = x.yAxes.push(new am4charts.ValueAxis());
        axis.events.on("disabled", (ev) => {
          x.series.each((series) => {
            if (!series.isHiding && !series.isHidden) {
              series.invalidate();
            }
          });
        });
        axis.events.on("enabled", (ev) => {
          x.series.each((series) => {
            if (!series.isHiding && !series.isHidden) {
              series.invalidate();
            }
          });
        });

        axis.tooltip.disabled = true;
        axis.renderer.minWidth = 35;
        axis.title.text = unit === '' ? name : (name + ' (' + unit + ')');
        axis.title.fill = color;
//        axis.renderer.opposite = opposite;

        axis.renderer.line.stroke = color;
        axis.renderer.labels.template.fill = color;
        axis.title.color = color;
        axis.disabled = hidden;
    }

//      const series = x.series.push(new am4charts.CandlestickSeries());
      const series = x.series.push(seriesType);
      series.name = name;
      series.dataFields.dateX = 'ts';
      series.dataFields.valueY = column;

      if (hidden) {
        series.hidden = true;
      }

      if (axis) { // anything but 'false' or undefined
        series.yAxis = axis;
        series.events.on("hidden", seriesToggleListener);
        series.events.on("shown", seriesToggleListener);
        // adjust global container height by axis:
        // https://www.amcharts.com/docs/v4/tutorials/auto-adjusting-chart-height-based-on-a-number-of-data-items/
      }

      series.tooltipText = tooltip;

      x.cursor = new am4charts.XYCursor();

      if (color) {
        series.stroke = color;
        series.fill = color;
      }

      return [axis, series];
    };

    const [vAxis, loadSeries] = addSeries('battery (load)', 'vl', false, 'V', am4core.color("#ffaa00"), new am4charts.LineSeries(), true, '{vl}V with WiFi active');
//    vAxis.max = 4.65;
    vAxis.min = 3.7;
//    loadSeries.strokeOpacity = 0;
    loadSeries.connect = false;
    const uploadBullet = newBullet();
    uploadBullet.adapter.add('fill', function(fill, target) {
      const r = (target.dataItem && target.dataItem.dataContext.upload) ? am4core.color("#00ff00") : am4core.color("#ff0000");
      return r;
    });
    loadSeries.bullets.push(uploadBullet);

    const [dummyAxis, vSeries] = addSeries('battery', 'vv', true, 'V', am4core.color("#aa8800"), new am4charts.LineSeries(), vAxis, '{vv}V with WiFi off');
//    vSeries.strokeOpacity = 0;
    vSeries.bullets.push(newBullet());
    vSeries.connect = false;

    const [percentAxis, percentSeries] = addSeries('battery', 'vp', false, '%', am4core.color("#aa8800"));
    percentAxis.max = 100;
//    percentSeries.strokeOpacity = 0;
    percentSeries.connect = false;
    percentSeries.bullets.push(newBullet());

/*    const [uploadAxis, uploadSeries] = addSeries('upload attempts', 'fails', false, '', am4core.color("#ff0000"), new am4charts.LineSeries(), false);
    uploadSeries.strokeOpacity = 0;
    const uploadBullet = new am4charts.CircleBullet();
    uploadBullet.circle.radius = 3;
    uploadBullet.adapter.add('dy', function(dy, target) {
      return target.dataItem ? .1 * x.plotContainer.measuredHeight - target.pixelY : dy;
    })
    uploadBullet.adapter.add('fill', function(fill, target) {
      // TODO check
      return fill;
    });
    uploadSeries.bullets.push(uploadBullet);
*/
    const [luxAxis, luxSeries] = addSeries('illuminance', 'lux', false, 'lux', colorSet.next(), new am4charts.ColumnSeries(), true, '{lux} lux @ {cct} K');
    luxAxis.logarithmic = true;
    luxAxis.min = .1;
    luxSeries.columns.template.width = am4core.percent(100);
    luxSeries.columns.template.strokeOpacity = 0;
    luxSeries.columns.template.adapter.add('fill', (fill, target) => {
      if (target.dataItem && target.dataItem.dataContext.c !== undefined) {
//        const hue = chroma(255*target.dataItem.dataContext.r / target.dataItem.dataContext.c, 255*target.dataItem.dataContext.g / target.dataItem.dataContext.c, 255*target.dataItem.dataContext.b / target.dataItem.dataContext.c);
//        console.log(hue);
//        hue.set('hsv.v', Math.min(100, Math.log(target.dataItem.dataContext.cct)));
//        return am4core.color(hue.hex());
//        const rgb = temperature(Math.round(target.dataItem.dataContext.cct)).set('rgb.r', 255);
        return am4core.color(colorTemperature2rgb(target.dataItem.dataContext.cct).hex());
      } else {
        return fill;
      }
    });

    let scrollbarX = new am4charts.XYChartScrollbar();
    scrollbarX.parent = x.bottomAxesContainer;
    // https://www.amcharts.com/docs/v4/tutorials/setting-minimal-width-of-scrollbar-thumb/
//    scrollbarX.thumb.minWidth = 20;
//    scrollbarX.thumb.maxWidth = 150;
    scrollbarX.series.push(luxSeries);
    // TODO dynamically enable/disable
    // https://www.amcharts.com/docs/v4/reference/grip/
    // https://www.amcharts.com/docs/v4/reference/slider/
//    scrollbarX.startGrip.disabled = true;
//    scrollbarX.endGrip.disabled = true;

    x.scrollbarX = scrollbarX;

/*
    const [cctAxis, cctSeries] = addSeries('color temperature', 'cct', true, 'Kelvin', colorSet.next());
    cctSeries.events.on("beforedatavalidated", function(ev) {
      // https://www.amcharts.com/docs/v4/tutorials/manipulating-chart-data/#Filtering_series_specific_data_items
      // only if there's at least 10 lux
      let source = ev.target.data;
      console.log(source);
      console.log(source.filter((el) => el.col1 >= 10));
    });
*/
    const [tempAxis, tempSeries] = addSeries('temperature', 'temp', true, '° C', am4core.color("#ffaa00"), new am4charts.LineSeries());
//    tempSeries.bullets.push(newBullet());

    const [dummyAxis2, humiditySeries] = addSeries('humidity', 'hum', false, '% humidity', am4core.color("#0000aa"), new am4charts.LineSeries(), percentAxis);
    humiditySeries.connect = false;
    // series.hidden = true

    x.dataSource.parser = new am4core.CSVParser();
    x.dataSource.parser.options.delimiter = ',';
    x.dataSource.parser.options.useColumnNames = true;
    x.dataSource.parser.options.dateFields = ['ts'];
    x.dataSource.parser.options.dateFormatter = new am4core.DateFormatter();
    x.dataSource.parser.options.dateFormatter.inputDateFormat = 'x';
    x.dataSource.parser.options.numberFields = ['bt', 'ss', 'c', 'r', 'g', 'b', 'v', 'temp', 'cct', 'lux','hum', 'fails'];

    x.dataSource.url = 'https://thiswasyouridea.com/thelightonmyroof/data.php?before=' + (Date.now() + 24*60*60000);
    // TODO https://www.amcharts.com/docs/v4/concepts/data/loading-external-data/#Modifying_URL_for_each_incremental_load
    //chart.dataSource.reloadFrequency = 5000;
    //chart.dataSource.incremental = true;
    /*chart.dataSource.adapter.add("url", function(url, target) {
      // "target" contains reference to the dataSource itself
      if (target.lastLoad) {
        url .= "?lastload=" + target.lastLoad.getTime();
      }
      return url;
    });*/


//    chart.current = x;

    return () => {
      x.dispose();
    };
//  }, []);
}

  render = () => {
    return (
      <div id="chartdiv" style={{ width: "100%", height: "100%" }}></div>
    );
  }
}

export default App;


/*
import {ResponsiveLine} from "@nivo/line"

    <div className="graph">
      <ResponsiveLine
        margin={{ top: 50, right: 50, bottom: 50, left: 50 }}
        xScale={{ type: 'time', format: 'native' }}
        yScale={{ type: 'linear', min: 0, max: 7000 }}
        layers={["grid", "axes", "lines", "markers", "legends"]}
        colors={{scheme: 'category10'}}
        axisLeft={{
          legend: 'color temperature (K)',
          legendPosition: 'middle',
        }}
        axisBottom={null}
        enableGridX={false}
        data={data}
        />
      </div>
    <div className="graph">
      <ResponsiveLine
        margin={{ top: 50, right: 50, bottom: 50, left: 50 }}
        xScale={{ type: 'time', format: 'native' }}
        yScale={{ type: 'log', min: .1, max: 100000 }}
        enablePoints={false}
        enableGridY={false}
        axisLeft={null}
        axisRight={{
          legend: 'illuminance (lux)',
          legendPosition: 'middle',
        }}
        axisBottom={{
          legend: 'time',
          legendPosition: 'middle',
          format: '%H:%M',
          tickValues: 'every 3 hours',
        }}
        data={logData}
        />
      </div>
*/
