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
    this.axisX = new Rickshaw.Graph.Axis.Time({
      // Disable this plugin due to formatting errors.
      // @see https://www.drupal.org/node/2354455
      // element: this.$axisX[0],
      orientation: 'bottom',
      graph: this.graph
    });
  }

  // Get the y-axis.
  liftGraph.prototype.setAxisY = function () {
    var orientation = $('html').attr('dir') == 'rtl' ? 'right' : 'left';

    this.axisY = new Rickshaw.Graph.Axis.LabeledY({
      element: this.$axisY[0],
      orientation: orientation,
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
    // Disable this plugin due to formatting errors.
    // @see https://www.drupal.org/node/2354455
    // this.$axisX = $('<div class="lift-graph-axis-x" role="presentation"></div>');
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
    // Rebuild the data.
    this.options = this.updateOptions();
    this.getData();
    this.getPalette();
    this.buildSeries(this.options.columnX, this.options.columnY, this.options.columnName);

    var graph = this.graph,
        series = this.series;

    // Update the series manually as Rickshaw does not do that by itself.
    $(graph.series).each(function(i){
        graph.series[i] = series[i];
    });

    // Update the Y axis label.
    this.axisY.label = this.columns[this.options.columnY - 1];

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
}(Drupal.jQuery);
