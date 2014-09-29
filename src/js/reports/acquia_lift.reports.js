/**
 * Functionality related specifically to the Acquia Lift reports.
 */

(function ($, Drupal) {
  "use strict";

  Drupal.behaviors.acquiaLiftReports = {
    attach: function (context, settings) {
      // Generate graphs and switches for the A/B statistics.
      $('.lift-statistics').once('acquiaLiftReports', function () {
        var $statistics = $(this),
            $data = $statistics.find('table[data-lift-statistics]'),
            campaign = $data.attr('data-acquia-lift-campaign'),
            $goalSelect = $('.acquia-lift-report-section-options .form-item-goal select'),
            $metricSelect = $('.acquia-lift-report-section-options .form-item-metric select'),
            metric = $metricSelect.val(),
            $metricOptions = $metricSelect.children('option'),
            // Find the tqble column that matches the metric value.
            metricColumn = function() {
              var $heads = $data.find('thead > tr > th'),
                  match = 0,
                  i = 1;
              while (i < $heads.length) {
                if ($($heads[i]).attr('data-conversion-metric') === metric) {
                  match = i + 1;
                  break;
                }
                i++;
              }

              return parseFloat(match);
            };

        // Format the initial data.
        $data.liftGraph();

        // Load a new report if the goal is changed.
        $goalSelect.change(function() {
          // Construct the GET path.
          var path = Drupal.settings.basePath + 'acquia_lift/reports/conversion',
              position = 0,
              args = {
                campaign: campaign,
                goal: $(this).val()
              };
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
          $.get(path, function (html) {
            var $html = $(html);
            $statistics.find('.lift-statistic-category').remove();
            $statistics.prepend($html);
            // Simultaneously replace $data with the new table.
            $data = $html.find('table[data-lift-statistics]');
            // Make sure the proper metric column is set and render the graph.
            $data.attr('data-liftgraph-columny', metricColumn()).liftGraph();
          });
        })

        // Attach a data column to a metric option.
        // Change the data fed to the y-axis and update the graph.
        $metricSelect.change(function () {
          // Set the new metric value.
          metric = $(this).val();

          // Change the data-liftgraph-columny attribute and rebuild the graph.
          $data.attr('data-liftgraph-columny', metricColumn())
            .liftGraph('update');
        })
      });
    }
  }

}(jQuery, Drupal));
