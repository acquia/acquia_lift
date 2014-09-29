/**
 * Functionality related specifically to the Acquia Lift reports.
 */

(function ($, Drupal) {
  "use strict";

  /**
   * Load a new graph report and format it as a graph.
   *
   * @params
   * - $container: A jQuery object that the report will be appended to.
   * - args: An object of filters to be applied to the results.
   *    - campaign: The machine name for the campaign data to load.
   *    - decision: The machine name for the desision to load.
   *    - goal: The machine name of the goal to load.
   *    - start: The unix timestamp for the start date of the report.
   *    - end: The unix timestamp for the end date of the report.
   */
  var acquiaLiftLoadReport = function ($container, args) {
    // Construct the GET path.
    var path = Drupal.settings.basePath + 'acquia_lift/reports/conversion',
        position = 0;
    for (var arg in args) {
      if (args.hasOwnProperty(arg)) {
        if (args[arg].length >= 0) {
          var prefix = position === 0 ? '?' : '&';
          path += prefix + arg + '=' + args[arg];
        }
      }
      position++;
    }

    // Get the new report and replace the existing report(s) with it.
    $.get(path, function (data) {
      var $data = $(data);
      $container.find('.lift-statistic-category').remove();
      $container.prepend($data);
      $data.find('table[data-lift-statistics]').liftGraph();
    });
  };

  Drupal.behaviors.acquiaLiftReports = {
    attach: function (context, settings) {
      // Generate graphs and switches for the A/B statistics.
      $('.lift-statistics').once('acquiaLiftReports', function () {
        var $statistics = $(this),
            $data = $statistics.find('table[data-lift-statistics]'),
            campaign = $data.attr('data-acquia-lift-campaign'),
            attr = typeof $data.attr('data-liftgraph-excluded') !== 'undefined' ? $data.attr('data-liftgraph-excluded').split(',') : [],
            variationColumn = $data.attr('data-liftgraph-columnname') - 1,
            xColumn = $data.attr('data-liftgraph-columnx') - 1,
            name = $data.attr('data-liftgraph-columnname') - 1,
            $goalSelect = $('.acquia-lift-report-section-options .form-item-goal select'),
            $metricSelect = $('.acquia-lift-report-section-options .form-item-metric select'),
            metric = $metricSelect.val(),
            $metricOptions = $metricSelect.children('option');

        // Format the initial data.
        $data.liftGraph();

        // The data-liftgraph-excluded attribute is a comma delimited list of
        // column numbers starting at 1. It needs a bit of extra processing
        // to make the individual strings from the split integers.
        for (var i = 0; i < attr.length; i++) {
          attr[i] = parseFloat(attr[i]);
        }

        // Load a new report if the goal is changed.
        $goalSelect.change(function() {
          acquiaLiftLoadReport($statistics, {campaign: campaign, goal: $(this).val()});
        })

        // Attach a data column to a metric option.
        // Change the data fed to the y-axis and update the graph.
        $metricSelect.change(function () {
          var value = $(this).val(),
              $heads = $data.find('thead > tr > th'),
              match = 0,
              i = 1;

          while (i < $heads.length) {
            if ($heads.attr('data-conversion-metric') === value) {
              match = i + 1;
              break;
            }
            i++;
          }

          console.log(match);

          $data.attr('data-liftgraph-columny', parseFloat(match))
            .liftGraph('update');
        })
      });
    }
  }

}(jQuery, Drupal));
