/**
 * Create an x-axis with formatted date labels.
 * ----------------------------------------------------------------------------
 */

Rickshaw.namespace('Rickshaw.Graph.Axis.TimeElement');

Rickshaw.Graph.Axis.TimeElement = function(args) {

  var self = this,
      berthRate = 0.10,
      time = args.timeFixture || new Rickshaw.Fixtures.Time();

  this.initialize = function(args) {

    this.graph = args.graph;
    this.elements = [];
    this.orientation = args.orientation || 'top';

    this.pixelsPerTick = args.pixelsPerTick || 75;
    if (args.ticks) this.staticTicks = args.ticks;
    if (args.tickValues) this.tickValues = args.tickValues;

    this.tickSize = args.tickSize || 4;
    this.ticksTreatment = args.ticksTreatment || 'plain';

    if (args.element) {

      this.element = args.element;
      this._discoverSize(args.element, args);

      this.vis = d3.select(args.element)
        .append("svg:svg")
        .attr('height', this.height)
        .attr('width', this.width)
        .attr('class', 'rickshaw_graph x_axis_d3');

      this.element = this.vis[0][0];
      this.element.style.position = 'relative';

      this.setSize({ width: args.width, height: args.height });

    } else {
      this.vis = this.graph.vis;
    }

    this.graph.onUpdate( function() { self.render() } );
  };

  this.setSize = function(args) {

    args = args || {};
    if (!this.element) return;

    this._discoverSize(this.element.parentNode, args);

    this.vis
      .attr('height', this.height)
      .attr('width', this.width * (1 + berthRate));

    var berth = Math.floor(this.width * berthRate / 2);
    this.element.style.left = -1 * berth + 'px';
  };

  this.appropriateTimeUnit = function() {

    var unit,
        units = time.units,
        domain = this.graph.x.domain(),
        rangeSeconds = domain[1] - domain[0];

    units.forEach( function(u) {
      if (Math.floor(rangeSeconds / u.seconds) >= 2) {
        unit = unit || u;
      }
    });

    return (unit || time.units[time.units.length - 1]);
  };

  this.render = function() {

    this.elements.forEach( function(e) {
      e.parentNode.removeChild(e);
    } );

    var unit = this.fixedTimeUnit || this.appropriateTimeUnit();

    if (this._renderWidth !== undefined && this.graph.width !== this._renderWidth) this.setSize({ auto: true });

    var axis = d3.svg.axis().scale(this.graph.x).orient(this.orientation);
    axis.tickFormat( args.tickFormat || function(x) { return unit.formatter(new Date(x * 1000)) } );
    if (this.tickValues) axis.tickValues(this.tickValues);

    this.ticks = this.staticTicks || Math.floor(this.graph.width / this.pixelsPerTick);

    var berth = Math.floor(this.width * berthRate / 2) || 0,
        transform;

    if (this.orientation == 'top') {
      var yOffset = this.height || this.graph.height;
      transform = 'translate(' + berth + ',' + yOffset + ')';
    } else {
      transform = 'translate(' + berth + ', 0)';
    }

    if (this.element) {
      this.vis.selectAll('*').remove();
    }

    this.vis
      .append("svg:g")
      .attr("class", ["x_ticks_d3", this.ticksTreatment].join(" "))
      .attr("transform", transform)
      .call(axis.ticks(this.ticks).tickSubdivide(0).tickSize(this.tickSize));

    var gridSize = (this.orientation == 'bottom' ? 1 : -1) * this.graph.height;

    this.graph.vis
      .append("svg:g")
      .attr("class", "x_grid_d3")
      .call(axis.ticks(this.ticks).tickSubdivide(0).tickSize(gridSize))
      .selectAll('text')
      .each(function() {
        this.parentNode.setAttribute('data-x-value', this.textContent)
      });

    this._renderHeight = this.graph.height;
  };

  this._discoverSize = function(element, args) {

    if (typeof window !== 'undefined') {

      var style = window.getComputedStyle(element, null),
          elementHeight = parseInt(style.getPropertyValue('height'), 10);

      if (!args.auto) {
        var elementWidth = parseInt(style.getPropertyValue('width'), 10);
      }
    }

    this.width = (args.width || elementWidth || this.graph.width) * (1 + berthRate);
    this.height = args.height || elementHeight || 40;
  };

  this.initialize(args);
};

/**
 * Add a label to the Y axis.
 * ----------------------------------------------------------------------------
 * The label will inherit the axis data name or can be explicitely defined.
 */

Rickshaw.namespace('Rickshaw.Graph.Axis.LabeledY');

Rickshaw.Graph.Axis.LabeledY = Rickshaw.Class.create(Rickshaw.Graph.Axis.Y, {
  initialize: function(args) {

    this.label = args.label || '';

    this.graph = args.graph;
    this.orientation = args.orientation || 'right';

    this.pixelsPerTick = args.pixelsPerTick || 75;
    if (args.ticks) this.staticTicks = args.ticks;
    if (args.tickValues) this.tickValues = args.tickValues;

    this.tickSize = args.tickSize || 4;
    this.ticksTreatment = args.ticksTreatment || 'plain';

    this.tickFormat = args.tickFormat || function(y) { return y };

    this.berthRate = 0.10;

    if (args.element) {

      this.element = args.element;
      this.vis = d3.select(args.element)
        .append("svg:svg")
        .attr('class', 'rickshaw_graph y_axis');

      this.element = this.vis[0][0];
      this.element.style.position = 'relative';

      this.setSize({ width: args.width, height: args.height });

    } else {
      this.vis = this.graph.vis;
    }

    var self = this;
    this.graph.onUpdate( function() { self.render() } );
  },
  _drawAxis: function(scale) {
    var axis = d3.svg.axis().scale(scale).orient(this.orientation);

    axis.tickFormat(this.tickFormat);
    if (this.tickValues) axis.tickValues(this.tickValues);

    if (this.orientation == 'left') {
      var berth = this.height * this.berthRate,
          transform = 'translate(' + this.width + ', ' + berth + ')';
    }

    if (this.element) {
      this.vis.selectAll('*').remove();
    }

    this.vis
      .append("svg:g")
      .attr("class", ["y_ticks", this.ticksTreatment].join(" "))
      .attr("transform", transform)
      .call(axis.ticks(this.ticks).tickSubdivide(0).tickSize(this.tickSize));

    label = this.vis
      .append("svg:text")
      .attr('class', 'y-axis-label')
      .attr('x', this.height / 4 * -1)
      .attr('y', this.width / 3)
      .attr('text-anchor', 'middle')
      .attr('transform', 'rotate(-90 50 50)')
      .text(this.label);

    return axis;
  }
});

/**
 * Create an tooltip showing data from all graphs.
 * ----------------------------------------------------------------------------
 */

Rickshaw.namespace('Rickshaw.Graph.ClickDetail');

Rickshaw.Graph.ClickDetail = Rickshaw.Class.create(Rickshaw.Graph.HoverDetail, {
  initialize: function(args) {

		var graph = this.graph = args.graph;

    this.tooltipTimeout;
    this.wiggleTimeout;
    this.tooltipPinned = false;

		this.xFormatter = args.xFormatter || function(x) {
			return new Date( x * 1000 ).toUTCString();
		};

		this.yFormatter = args.yFormatter || function(y) {
			return y === null ? y : y.toFixed(2);
		};

		var element = this.element = document.createElement('div');
		element.className = 'detail inactive';

		this.visible = true;
		graph.element.appendChild(element);

		this.lastEvent = null;
		this._addListeners();

		this.onShow = args.onShow;
		this.onHide = args.onHide;
		this.onRender = args.onRender;

		this.formatter = args.formatter || this.formatter;

	},
  formatter: function(series, x, y, formattedX, formattedY, d) {
    var self = this,
        options = this.graph.rawData.options,
        columns = this.graph.rawData.columns,
        xKey = columns[options.columnX - 1],
        yKey = columns[options.columnY - 1],
        nameKey = columns[options.columnName - 1],
        data = this.graph.rawData.groups,
        time = new Date(x * 1000),
        months = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'April', 'September', 'June', 'July', 'August', 'September', 'October', 'November', 'December']
        humanDate = months[time.getUTCMonth()] + ' ' + time.getDate() + ', ' + time.getFullYear()
        head = function () {
          var date = '<th>' + humanDate + '</th>',
              variations = '';

          for (var i = 0; i < self.graph.series.length; i++) {
            variations += '<th style="background-color: ' + self.graph.series[i].color + ';">' + self.graph.series[i].shortName + '</th>';
          }

          return '<thead><tr>' + date + variations + '</tr></thead>';
        },
        row = function (data) {
          var output = '<td class="label">' + data.property + '</td>';

          for (var i = 0; i < data.data.length; i++) {
            if (typeof data.data[i].active != 'undefined' && data.data[i].active == true) {
              output = output + '<td class="active">' + data.data[i].value + '</td>';
            }
            else {
              output += '<td>' + data.data[i].value + '</td>';
            }
          }

          return '<tr>' + output + '</tr>';
        },
        rows = function () {
          var output = '';

          // Build each row of data.
          for (var key = 0; key < data[series.name].length; key++) {
            if (data[series.name][key][xKey] == x) {
              for (var property in data[series.name][key]) {
                if (data[series.name][key].hasOwnProperty(property) && property != xKey && property != nameKey && property != nameKey + ' label') {
                  var rowData = {property: property, data: []};

                  for (var group in data) {
                    if (data.hasOwnProperty(group)) {
                      var information = {value: data[group][key][property]};
                      if (series.name == data[group][key][nameKey]) {
                        information.active = true;
                      }
                      rowData.data.push(information);
                    }
                  }

                  output += row(rowData);
                };
              };
            };
          };

          return '<tbody>' + output + '</tbody>';
        };

    return '<div class="lift-graph-detail"><table class="lift-graph-detail-data">' + head() + rows() + '</table></div>';
  },
  render: function (args) {
    var $ = jQuery,
        graph = this.graph,
        points = args.points,
        point = points.filter( function(p) { return p.active } ).shift();

    if (point.value.y === null) return;

    var formattedXValue = point.formattedXValue,
        formattedYValue = point.formattedYValue;

    this.element.innerHTML = '';
    this.element.style.left = graph.x(point.value.x) + 'px';

    var item = document.createElement('div');

    item.className = 'item';
    item.style.maxWidth = $(graph.element).width() + 'px';

    // invert the scale if this series displays using a scale
    var series = point.series,
        actualY = series.scale ? series.scale.invert(point.value.y) : point.value.y;

    item.innerHTML = this.formatter(series, point.value.x, actualY, formattedXValue, formattedYValue, point);

    this.element.appendChild(item);

    var dot = document.createElement('div'),
        topPosition = this.graph.y(point.value.y0 + point.value.y);

    dot.className = 'dot';
    dot.style.top = topPosition + 'px';
    dot.style.borderColor = series.color;

    this.element.appendChild(dot);

    if (point.active) {
      item.classList.add('active');
      dot.classList.add('active');
    }

    var $item = $(item),
        itemWidth = parseInt($item.innerWidth()),
        itemHeight = parseInt($item.innerHeight()),
        itemMargin = parseInt($item.css('margin-top')) + parseInt($item.css('margin-bottom'));

    item.style.top = Math.round(topPosition - itemHeight - itemMargin) + 'px';
    item.style.left = Math.round(itemWidth / 2) * -1 + 'px';

    // Assume left alignment until the element has been displayed and
    // bounding box calculations are possible.
    var alignables = [item];

    alignables.forEach(function(el) {
      el.classList.add('bottom');
      el.classList.add('center');
    });

    this.show();

    var alignment = this._calcLayout(item);

    if (alignment.left > alignment.right) {
      item.style.left = 'auto';
      item.classList.remove('center');
      item.classList.remove('right');
      item.classList.add('left');
    }

    if (alignment.right > alignment.left) {
      item.style.left = 'auto';
      item.classList.remove('center');
      item.classList.remove('left');
      item.classList.add('right');
    }

    if (alignment.top === 0) {
      item.style.top = topPosition + 'px';
      item.classList.remove('bottom');
      item.classList.add('top');
    }

    if (typeof this.onRender == 'function') {
      this.onRender(args);
    }
  },
  _calcLayout: function(element) {
    var layout = {top: 0, right: 0, bottom: 0, left: 0},
        parentRect = this.element.parentNode.getBoundingClientRect(),
        rect = element.getBoundingClientRect();

    if (rect.top > parentRect.top) {
      layout.top += rect.top - parentRect.top;
    }

    if (rect.bottom < parentRect.bottom) {
      layout.bottom += parentRect.bottom - rect.bottom;
    }

    if (rect.right > parentRect.right) {
      layout.right += rect.right - parentRect.right;
    }

    if (rect.left < parentRect.left) {
      layout.left += parentRect.left - rect.left;
    }

    return layout;
  },
  _addListeners: function() {
    var $ = jQuery,
        self = this,
        $element = $(this.element);

		this.graph.element.addEventListener(
			'mousemove',
			function(e) {
        if (this.tooltipPinned === false && $element.hasClass('inactive')) {
          clearTimeout(this.tooltipTimeout);
          this.tooltipTimeout = window.setTimeout(function () {
            self.visible = true;
            self.update(e);
          }, 350);
        }
        else if (this.tooltipPinned === false) {
          this.wiggleTimeout = window.setTimeout(function () {
            if (!$element.is(':hover')) {
              self.hide();
            }
          }, 120);
        }
			}.bind(this),
			false
		);

    this.graph.element.addEventListener(
      'click',
      function(e) {
        if (this.tooltipPinned === false) {
          this.tooltipPinned = true;
          clearTimeout(this.tooltipTimeout);
          this.visible = true;
          this.update(e);
        }
        else {
          this.tooltipPinned = false;
          this.hide();
        }
      }.bind(this),
      false
    );

		this.graph.onUpdate( function() { this.update() }.bind(this) );

		this.graph.element.addEventListener(
			'mouseout',
			function(e) {
        if (this.tooltipPinned === false) {
          var self = this;

  				if (e.relatedTarget && !(e.relatedTarget.compareDocumentPosition(this.graph.element) & Node.DOCUMENT_POSITION_CONTAINS)) {
            clearTimeout(this.tooltipTimeout);
            this.tooltipTimeout = window.setTimeout(function () {
              self.hide();
            }, 500);
  				}
        }
			}.bind(this),
			false
		);
	}
});

/**
 * Append the legend to the first column of a table.
 * ----------------------------------------------------------------------------
 */

Rickshaw.namespace('Rickshaw.Graph.TableLegend');

Rickshaw.Graph.TableLegend = Rickshaw.Class.create(Rickshaw.Graph.Legend, {

  initialize: function(args) {
    this.element = args.element;
    this.graph = args.graph;

    this.render();

    // we could bind this.render.bind(this) here
    // but triggering the re-render would lose the added
    // behavior of the series toggle
    this.graph.onUpdate( function() {} );
  },
  render: function() {
    var $ = jQuery,
        self = this,
        $label = $(this.element).find('thead > tr > th:first-child');

    this.lines = [];

    var series = this.graph.series
      .map( function(s) { return s } );

    series.forEach( function(s) {
      self.addLine(s);
    } );
  },
  addLine: function (series) {
    var $ = jQuery,
        self = this;

    $(this.element).find('tbody > tr').each(function (index, row) {
      var $cell = $(row).find('td:first-child');

      if ($(row).find('td:first-child').text() == series.name) {

        $cell.addClass('legend line');
        if (series.disabled) {
          $cell.addClass('disabled');
        }
        if (series.className) {
          d3.select($cell[0]).classed(series.className, true);
        }

        var label = document.createElement('span');
        label.className = 'label';
        label.innerHTML = (series.shortName || series.name) + ': ';

        $cell.prepend(label);

        $cell[0].series = series;

        var swatch = document.createElement('div');
        swatch.className = 'swatch';
        swatch.style.backgroundColor = series.color;

        $cell.prepend(swatch);

        if (series.noLegend) {
          $cell.css('display', 'none');
        }

        var _line = { element: $cell[0], series: series };
        if (self.shelving) {
          self.shelving.addAnchor(_line);
          self.shelving.updateBehaviour();
        }
        if (self.highlighter) {
          self.highlighter.addHighlightEvents(_line);
        }
        self.lines.push(_line);
        return $cell[0];
      }
    });
  }
});

/**
 * Create a graph using data from a table element and Rickshaw.
 * ----------------------------------------------------------------------------
 */

+function ($) {
  'use strict';

  // Assemble the object.
  var liftGraph = function (element, options) {
    this.type =
    this.options =
    this.enabled =
    this.$element = null

    this.init('liftGraph', element, options);
  }

  // Define the plugin defaults.
  liftGraph.DEFAULTS = {
    columnName: 1,
    columnX: 2,
    columnY: 3,
    scheme: null,
    renderer: null,
    width: null,
    height: null,
    min: null,
    max: null,
    padding: null,
    interpolation: 'linear',
    stack: null
  };

  // Initialize the plugin functionality.
  liftGraph.prototype.init = function (type, element, options) {
    this.type = type
    this.$element = $(element)
    this.options = this.getOptions(options)
    this.enabled = true

    this.render();
  }

  // Enable the graph.
  liftGraph.prototype.enable = function () {
    this.enabled = true;
  }

  // Disable the graph.
  liftGraph.prototype.disable = function () {
    this.enabled = false;
  }

  // Get the option value of a data attribute.
  liftGraph.prototype.dataAttr = function (key) {
    return this.$element.attr('data-' + this.type + '-' + key);
  }

  // Get default values.
  liftGraph.prototype.getDefaults = function () {
    return liftGraph.DEFAULTS;
  }

  // Get options.
  liftGraph.prototype.getOptions = function (options) {
    options = $.extend({}, this.getDefaults(), options);
    for (var i in options) {
      options[i] = this.dataAttr(i) || options[i];
    }
    return options;
  }

  // Update options.
  liftGraph.prototype.updateOptions = function () {
    var options = this.options;
    for (var i in options) {
      options[i] = this.dataAttr(i) || options[i];
    }
    return options;
  }

  // Collect the data from the table.
  liftGraph.prototype.getData = function () {
    var columns = [],
        data = [],
        grouped = {};

    // Get the column names from the thead of the table.
    this.$element.find('thead > tr > th').each(function (i) {
      columns[i] = $(this).text();
    });

    // Structure the data in the table.
    this.$element.find('tbody > tr').each(function (row) {
      data[row] = {};

      $(this).children('td').each(function (i) {
        data[row][columns[i]] = $(this).text();

        var label = $(this).attr('data-acquia-lift-variation-label');

        if (label && label.length > 0) {
          data[row][columns[i] + ' label'] = label;
        }
      });
    });

    // Get the grouping column name.
    var groupName = columns[this.options.columnName - 1];

    // Group data by a defined label..
    for (var i = 0; i < data.length; i++) {
      if (typeof grouped[data[i][groupName]] == 'undefined') {
        grouped[data[i][groupName]] = [];
      }
      grouped[data[i][groupName]].push(data[i]);
    }

    this.columns = columns;
    this.groups = grouped;
  }

  // Build graphing coordinates.
  liftGraph.prototype.buildSeries = function (columnX, columnY, columnName) {
    var groups = this.groups,
        xKey = this.columns[columnX - 1],
        yKey = this.columns[columnY - 1],
        nameKey = this.columns[columnName - 1],
        series = [],
        results = $('.lift-graph-results > tbody > tr > td:first-child'),
        counter = 0;

    for (var key in groups) {
      if (groups.hasOwnProperty(key)) {
        var data = [];

        for (var i = 0; i < groups[key].length; i++) {
          data.push({
            x: parseFloat(groups[key][i][xKey]),
            y: parseFloat(groups[key][i][yKey])
          });
        }

        series[counter] = {
          name: key,
          color: this.palette.color(),
          data: data,
          shortName: groups[key][0][nameKey + ' label'] || key
        };

        counter++;
      }
    }

    this.series = series;
  }

  // Get the optimal number of palette colors.
  liftGraph.prototype.getPalette = function () {
    var color = new Rickshaw.Fixtures.Color(),
        scheme = this.options.scheme || 'colorwheel',
        configuration = {scheme: scheme},
        count = 0;

    // Get the number of individual graphs.
    for (var key in this.groups) {
      if (this.groups.hasOwnProperty(key)) {
        count++;
      }
    }

    if (color.schemes[scheme].length < count) {
      configuration.interpolatedStopCount = count;
    }

    this.palette = new Rickshaw.Color.Palette(configuration);
  }

  // Get the graph object.
  liftGraph.prototype.getGraph = function () {
    var configuration = {
      element: this.$graph[0],
      series: this.series
    };

    // Set the custom renderer.
    if (this.options.renderer != null) {
      configuration.renderer = this.options.renderer;
    }

    // Set the custom width.
    if (this.options.width != null) {
      configuration.width = this.options.width;
    }
    else {
      configuration.width = this.$graph.width();
    }

    // Set the custom height.
    if (this.options.height != null) {
      configuration.height = this.options.height;
    }
    else {
      configuration.height = this.$graph.height();
    }

    // Set the custom min.
    if (this.options.max != null) {
      configuration.max = this.options.max;
    }

    // Set the custom max.
    if (this.options.min != null) {
      configuration.min = this.options.min;
    }

    // Set the custom padding.
    if (this.options.padding != null) {
      var data = this.options.padding.split(' '),
          dataLength = data.length,
          padding = {},
          position = ['top', 'right', 'bottom', 'left'];

      for (var i = 0; i < dataLength; i++) {
        padding[position[i]] = parseFloat(data[i]);
      }

      configuration.padding = padding;
    }

    // Set the custom interpolation.
    if (this.options.interpolation != null) {
      configuration.interpolation = this.options.interpolation;
    }

    // Set custom stack.
    if (this.options.stack != null) {
      configuration.stack = this.options.stack;
    }

    this.graph = new Rickshaw.Graph(configuration);

    // Add the raw data to the
    this.graph.rawData = this;
  }

  // Get the x-axis.
  liftGraph.prototype.setAxisX = function () {
    this.axisX = new Rickshaw.Graph.Axis.TimeElement({
      element: this.$axisX[0],
      orientation: 'bottom',
      graph: this.graph
    });
  }

  // Get the y-axis.
  liftGraph.prototype.setAxisY = function () {
    this.axisY = new Rickshaw.Graph.Axis.LabeledY({
      element: this.$axisY[0],
      orientation: 'left',
      label: this.columns[this.options.columnY - 1],
      graph: this.graph
    });
  }

  // Get the legend.
  liftGraph.prototype.setLegend = function () {
    this.legend = new Rickshaw.Graph.TableLegend({
      element: this.$legend[0],
      graph: this.graph
    });
  }

  // Get the range slider.
  liftGraph.prototype.setRangeSlider = function () {
    this.rangeSlider = new Rickshaw.Graph.RangeSlider({
      element: this.$rangeSlider[0],
      graph: this.graph
    });
  }

  // Activate hover details.
  liftGraph.prototype.setHoverDetail = function () {
    this.hoverDetail = new Rickshaw.Graph.ClickDetail({
      graph: this.graph
    });
  }

  // Highlight a series when hovering on legend.
  liftGraph.prototype.setSeriesHighlight = function () {
    this.seriesHighlight = new Rickshaw.Graph.Behavior.Series.Highlight({
      graph: this.graph,
      legend: this.legend
    });
  }

  // Allow a user to togle graph data via the legend.
  liftGraph.prototype.setSeriesToggle = function () {
    this.$legend.addClass('toggle-enabled');
    this.seriesToggle = new Rickshaw.Graph.Behavior.Series.Toggle({
      graph: this.graph,
      legend: this.legend
    });
  }

  // Format the elements of the graph.
  liftGraph.prototype.build = function () {
    this.$graph = $('<div class="lift-graph-graph" role="presentation"></div>');
    this.$axisY = $('<div class="lift-graph-axis-y" role="presentation"></div>');
    this.$axisX = $('<div class="lift-graph-axis-x" role="presentation"></div>');
    this.$legend = this.$element.siblings('.lift-graph-result').children('table.lift-graph-result-data');
    this.$rangeSlider = $('<div class="lift-graph-range-slider" role="presentation"></div>');

    this.$element.addClass('lift-graph-table')
      .wrap('<div class="lift-graph-container"></div>')
      .before(this.$axisY)
      .before(this.$graph)
      .before(this.$axisX)
      .before(this.$rangeSlider);
  }

  // Hide the table.
  liftGraph.prototype.hideTable = function () {
    this.$element.hide();
  }

  // Show the table.
  liftGraph.prototype.showTable = function () {
    this.$element.show();
  }

  // Render the graph.
  liftGraph.prototype.render = function () {
    this.getData();
    this.getPalette();
    this.buildSeries(this.options.columnX, this.options.columnY, this.options.columnName);
    this.build();
    this.getGraph();
    this.setAxisX();
    this.setAxisY();
    this.setLegend();
    this.setHoverDetail();
    this.graph.render();
    this.hideTable();
  }

  liftGraph.prototype.update = function () {
    this.options = this.updateOptions();
    this.getData();
    this.getPalette();
    this.buildSeries(this.options.columnX, this.options.columnY, this.options.columnName);

    var graph = this.graph,
        series = this.series;

    $(graph.series).each(function(i){
        graph.series[i] = series[i];
    });

    this.graph.update();
  }

  // Define the jQuery plugin.
  var old = $.fn.railroad;

  $.fn.liftGraph = function (option) {
    return this.each(function () {
      var $this = $(this),
          data = $this.data('lift.graph');

      if (!data) $this.data('lift.graph', (data = new liftGraph(this, option)));
      if (typeof option == 'string') data[option]($this);
    });
  }

  $.fn.liftGraph.Constrictor = liftGraph;

  $.fn.liftGraph.noConflict = function () {
    $.fn.liftGraph = old;
    return this;
  }
}(jQuery);

//# sourceMappingURL=acquia_lift.report.ab.js.map