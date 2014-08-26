/**
 * Append the legend to the first column of a table.
 * ----------------------------------------------------------------------------
 */

Rickshaw.namespace('Rickshaw.Graph.TableLegend');

Rickshaw.Graph.TableLegend = Rickshaw.Class.create(Rickshaw.Graph.Legend, {
  // className: 'rickshaw_legend',

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

    // Add a new column label for the legend.
    $(this.element).find('thead > tr').prepend('<th>Legend</th>');

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
      if ($(row).find('td:first-child').text() == series.name) {
        var line = document.createElement('td');

        $(row).prepend(line);

        line.className = 'legend line';
        if (series.disabled) {
          line.className += ' disabled';
        }
        if (series.className) {
          d3.select(line).classed(series.className, true);
        }

        var swatch = document.createElement('div');
        swatch.className = 'swatch';
        swatch.style.backgroundColor = series.color;

        line.appendChild(swatch);

        var label = document.createElement('span');
        label.className = 'label';
        label.innerHTML = series.shortName || series.name;

        line.appendChild(label);

        line.series = series;

        if (series.noLegend) {
          line.style.display = 'none';
        }

        var _line = { element: line, series: series };
        if (self.shelving) {
          self.shelving.addAnchor(_line);
          self.shelving.updateBehaviour();
        }
        if (self.highlighter) {
          self.highlighter.addHighlightEvents(_line);
        }
        self.lines.push(_line);
        return line;
      }
    });
  }
});
