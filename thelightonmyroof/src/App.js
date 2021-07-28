import React, { Component, useLayoutEffect, useRef } from 'react';

import chroma from 'chroma-js';

import * as am4core from "@amcharts/amcharts4/core";
import * as am4charts from "@amcharts/amcharts4/charts";
import am4themes_animated from "@amcharts/amcharts4/themes/animated";

// https://www.amcharts.com/docs/v4/tutorials/creating-themes/
function am4themes_myTheme(target) {
  if (target instanceof am4core.InterfaceColorSet) {
    target.setFor("background", am4core.color("#000000"));
    target.setFor("fill", am4core.color("#000000"));
    target.setFor("grid", am4core.color("#aaaaaa"));
    target.setFor("text", am4core.color("#eeeeee"));
  }
}
am4core.useTheme(am4themes_animated);
am4core.useTheme(am4themes_myTheme);

const newBullet = (addLine = false) => {
  const c = new am4charts.CircleBullet();
  c.circle.radius = 2.5;
  c.circle.strokeOpacity = 0;
  if (addLine) {
    // if bootCount == 0, add a line like
    // https://github.com/amcharts/amcharts4/issues/1517
    const line = c.createChild(am4core.Line);
    line.x1 = 0;
    line.y1 = -20;
    line.x2 = 0;
    line.y2 = 20;
    line.propertyFields.stroke = "color";
    line.strokeDasharray = "3,3";
    line.strokeOpacity = 0;
    line.adapter.add('strokeOpacity', (opacity, target) => (target.dataItem.dataContext.bt === 0) ? 1 : 0);
    // TODO mebbe https://www.amcharts.com/docs/v4/tutorials/ordering-zindex-of-series-lines-and-bullets/#Mixed_order_of_lines_and_bullets
  }
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

const lux2rgb = (lux, c, rgb, luxCap = 20) => {
  const proportion = Math.min(1, Math.log(2.718282 + lux / luxCap) - 1);
//  const proportion = Math.min(1, lux / luxCap); // 5000 is good
//  rgb = [rgb[0]/.2126, rgb[1]/.7152, rgb[2]/.0722]
//  const ir = (rgb[0] + rgb[1] + rgb[2] - c ) / 2;
  const mx = Math.max(...rgb);
  return 'rgb(' + rgb.map(x => Math.round(255*proportion*x/mx)).join(',') + ')';
};

const cctAdapter = (fill, target) => {
  if (target.dataItem && target.dataItem.dataContext.c !== undefined) {
//        const rgb = temperature(Math.round(target.dataItem.dataContext.cct)).set('rgb.r', 255);
    return am4core.color(colorTemperature2rgb(target.dataItem.dataContext.cct).hex());
  } else {
    return fill;
  }
};

const rgbAdapter = (fill, target) => {
  if (target.dataItem && target.dataItem.dataContext.c !== undefined) {
    return am4core.color(lux2rgb(target.dataItem.dataContext.lux, target.dataItem.dataContext.c, [target.dataItem.dataContext.r, target.dataItem.dataContext.g, target.dataItem.dataContext.b]));
  } else {
    return fill;
  }
};

class App extends Component {

componentDidMount() {
//  const chart = useRef(null);


// useLayoutEffect(() => {
    let x = am4core.create("chartdiv", am4charts.XYChart);
    x.dateFormatter = new am4core.DateFormatter();
    x.dateFormatter.timezoneOffset = 8*60;

    x.legend = new am4charts.Legend();
    x.zoomOutButton.valign = 'bottom';
//    x.zoomOutButton.parent = x.tooltipContainer;
    x.tooltip.label.textAlign = 'middle';

    var prevData = [];
    var appendDataIfAny;
    appendDataIfAny = (ev) => {
      button.disabled = ev.target.data.length === 0;
      if (button.disabled) {
        if (prevData.length === 0) {
          x.events.once("beforedatavalidated", appendDataIfAny);
        } else {
          ev.target.data = prevData;
        }
        return;
      }
      for (let i = ev.target.data.length - 1; i > 0; i--) {
        if (ev.target.data[i].attempt) {
          ev.target.data[i].upload = ev.target.data[i].attempt && (!ev.target.data[i].code);
          if (ev.target.data[i].attempt) {
            // >= 100: https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/201
            // < 0: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.h#L46
            // >= 0: https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html#check-return-codes
            const url = ev.target.data[i].code >= 100 ? ('https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/' + ev.target.data[i].code) :
              (ev.target.data[i].code < 0) ? ('https://github.com/esp8266/Arduino/blob/929f0fb63c86db9fd9ea70132a4671fce63f21d8/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.h#L46-L57') :
              'https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html#check-return-codes';
            ev.target.data[i].uploadmsg = ev.target.data[i].code ?
            (ev.target.data[i].attempt + 'th consecutive failed upload attempt (<a href="' + url + '"  target="_blank">' + ev.target.data[i].code + '</a>)') :
            ('successful upload on attempt #' + ev.target.data[i].attempt);
          }
          // TODO FIXME if bootcount is 0 set to null so that a gap in the data is drawn
          ev.target.data[i].vl = ev.target.data[i].v / 200;
//          ev.target.data[i].vl = ev.target.data[i].bt === 0 ? null : ev.target.data[i].v / 200;
          // rough heuristic:
//          ev.target.data[i].vp = 200 * (ev.target.data[i].vl - 3.7);
          // better: https://i.stack.imgur.com/LV91V.gif
          const thresholds = [3.4, 3.68, 3.74, 3.8, 3.95, 4.2, 1000]; // first one could also be 3.0 for completely dead
          const th = Math.max(1, thresholds.findIndex((el) => ev.target.data[i].vl < el));
          ev.target.data[i].vp = Math.min(100, Math.round(20 * (th - 1 + (ev.target.data[i].vl - thresholds[th-1]) / (thresholds[th] - thresholds[th-1]))));
//          ev.target.data[i].col = 
        } else {
          ev.target.data[i].vv = ev.target.data[i].v / 200;
        }
      }
      ev.target.data = ev.target.data.concat(prevData);
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
      x.dataSource.url = 'https://thiswasyouridea.com/thelightonmyroof/d.php?before=' + x.data[0].ts.getTime();
      x.dataSource.load();
    });

    x.events.once('beforedatavalidated', appendDataIfAny);

    // chart.legend.itemContainers.template.focusable = false;
    // chart.legend.itemContainers.template.cursorOverStyle = am4core.MouseCursorStyle.default;
    x.paddingRight = 20;

    x.leftAxesContainer.layout = 'vertical';
    x.rightAxesContainer.layout = 'vertical';

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

    const addSeries = (name, column, hidden = false, unit = '', color = undefined, seriesType = new am4charts.LineSeries(), axis = true, tooltip = '{' + column + '}' + unit, opposite = false) => {
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
        axis.renderer.opposite = opposite;
        axis.marginTop = 10;
        axis.marginBottom = 10;
        axis.align = opposite ? 'left' : 'right';

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

      series.tooltipHTML = tooltip;

      x.cursor = new am4charts.XYCursor();

      if (color) {
        series.stroke = color;
        series.fill = color;
      }

      return [axis, series];
    };

    const [vAxis, loadSeries] = addSeries('battery charge', 'vl', false, 'V', am4core.color("#ff5500"), new am4charts.LineSeries(), true, '{uploadmsg} @ {ts.formatDate("HH:mm:ss")}<br>battery @ {vl} V (~{vp}%)');
//    vAxis.max = 4.65;
    vAxis.min = 3.7;
//    loadSeries.strokeOpacity = 0;
    // TODO FIXME draw gaps at every reboot
//    loadSeries.autoGapCount = Infinity;
//    loadSeries.connect = false;
    loadSeries.hiddenInLegend = true;
    // if bootCount == 0, add a line like
    // https://github.com/amcharts/amcharts4/issues/1517
    const uploadBullet = newBullet(true);
    uploadBullet.adapter.add('fill', function(fill, target) {
      const r = (target.dataItem && target.dataItem.dataContext.upload) ? am4core.color("#0c0") : am4core.color("#c00");
      return r;
    });
    loadSeries.bullets.push(uploadBullet);
    loadSeries.tooltip.getFillFromObject = false;
    loadSeries.tooltip.adapter.add('x', (x, target) => {
      target.background.fill = (target.dataItem && target.dataItem.dataContext && target.dataItem.dataContext.upload) ? am4core.color("#080") : am4core.color("#800");
      return x;
    });

    const [luxAxis, luxSeries] = addSeries('illuminance', 'lux', false, 'lux', colorSet.next(), new am4charts.ColumnSeries(), true, '{lux} lux @ {cct} K');
    luxSeries.legendSettings.labelText = '[bold]toggle color temperature / true color[/bold]';
    luxAxis.logarithmic = true;
    luxAxis.min = .08;
    luxSeries.columns.template.width = am4core.percent(100);
    luxSeries.columns.template.adapter.add('fill', cctAdapter);
    luxSeries.columns.template.strokeOpacity = 0;

    luxSeries.clustered = false;

    const [dummyAxis, colorSeries] = addSeries('illuminance', 'lux', true, 'lux', '#aaa', new am4charts.ColumnSeries(), luxAxis, '{lux} lux @ {cct} K');
    colorSeries.clustered = false;

    colorSeries.hiddenInLegend = true;
    colorSeries.columns.template.width = am4core.percent(100);
    colorSeries.columns.template.adapter.add('fill', rgbAdapter);

/*    x.legend.itemContainers.template.togglable = false;
    x.legend.itemContainers.template.events.on('hit', function(ev) {
      if (ev.target.dataItem.dataContext.dataFields.valueY !== 'lux' && (ev.target.dataItem.dataContext.isHiding || ev.target.dataItem.dataContext.isHidden)) {
        ev.target.dataItem.dataContext.show();
      } else {
        ev.target.dataItem.dataContext.hide();
      }
    });
*/
    luxSeries.events.on('hidden', () => {
      colorSeries.show();
    });
    luxSeries.events.on('shown', () => {
      colorSeries.hide();
/*      luxSeries.invalidate();
      luxSeries.columns.template.adapter.remove('fill');
      trueColor = !trueColor
      console.log(trueColor);
      luxSeries.columns.template.adapter.add('fill', trueColor ? rgbAdapter : cctAdapter);
      luxSeries.columns.template.strokeOpacity = trueColor ? 1 : 0;
      luxSeries.show(); */
    });

    luxSeries.show();

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

//    const [dummyAxis, vSeries] = addSeries('battery', 'vv', true, 'V', am4core.color("#aa8800"), new am4charts.LineSeries(), vAxis, '{vv}V with WiFi off');
//    vSeries.strokeOpacity = 0;
//    vSeries.bullets.push(newBullet());
//    vSeries.connect = false;

//    const [percentAxis, percentSeries] = addSeries('battery', 'vp', false, '%', am4core.color("#aa8800"));
//    percentAxis.max = 100;
//    percentSeries.strokeOpacity = 0;
//    percentSeries.connect = false;
//    percentSeries.bullets.push(newBullet());

/*    const [uploadAxis, uploadSeries] = addSeries('upload attempts', 'attempt', false, '', am4core.color("#ff0000"), new am4charts.LineSeries(), false);
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

    const [percentAxis, humiditySeries] = addSeries('humidity', 'hum', true, '%', am4core.color("#57f"), new am4charts.LineSeries(), true, '{hum}% humidity', true);
    percentAxis.min = 40;
//    humiditySeries.connect = false;
    //const [tempAxis, tempSeries] =
    addSeries('temperature', 'temp', true, '° C', am4core.color("#fa0"), new am4charts.LineSeries(), true, undefined, true);
//    tempSeries.bullets.push(newBullet());

    x.scrollbarX = scrollbarX;

    x.dataSource.parser = new am4core.CSVParser();
    x.dataSource.parser.options.delimiter = ',';
    x.dataSource.parser.options.useColumnNames = true;
    x.dataSource.parser.options.dateFields = ['ts'];
    x.dataSource.parser.options.dateFormatter = new am4core.DateFormatter();
    x.dataSource.parser.options.dateFormatter.inputDateFormat = 'x';
    x.dataSource.parser.options.numberFields = ['bt', 'ss', 'c', 'r', 'g', 'b', 'v', 'temp', 'cct', 'lux','hum', 'attempt'];

    x.dataSource.url = 'https://thiswasyouridea.com/thelightonmyroof/d.php?before=' + (Date.now() + 24*60*60000);
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
