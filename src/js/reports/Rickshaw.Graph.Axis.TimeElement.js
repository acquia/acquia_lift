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
