<?php

/**
 * Class AcquiaLiftReportBase
 *
 * This class is provided as a base reporting class for displaying report markup
 * regardless of where reporting data is coming from. It provides functions for
 * building out the markup of overview and experiment reports, as well as some
 * helper methods for formatting numbers, and methods for getting and setting
 * the report configuration. Reporting classes should extend this base class
 * and use it to generate the markup, while implementing their own logic for
 * getting and manipulating the actual reporting data.
 */
abstract class AcquiaLiftReportBase {

  /**
   * The Acquia Lift agent instance for reporting on.
   *
   * @var AcquiaLiftAgent
   */
  protected $agent;

  /**
   * The confidence measure for determining statistical significance.
   */
  protected $confidence_measure = 95;

  /**
   * The configuration options for the report.
   *
   * @var array;
   */
  protected $reportConfig;

  /**
   * Holds the reporting data that has been loaded and extracted.
   *
   * @var array;
   */
  protected $loadedReports;

  /**
   * Boolean indicating whether the report has data.
   *
   * @var bool
   */
  protected $hasData = TRUE;

  /**
   * Constructs an AcquiaLiftReport object
   *
   * @param PersonalizeAgentInterface $agent
   *   The agent the report is for.
   *
   * @param array $options
   *   An array of report options.
   */
  function __construct(PersonalizeAgentInterface $agent, $options) {
    $this->agent = $agent;
    $confidence_measure = $options['confidence_measure'];
    if ($confidence_measure < 0) {
      $confidence_measure = 0;
    }
    if ($confidence_measure > 100) {
      $confidence_measure = 100;
    }
    $this->confidence_measure = $confidence_measure;
    unset($options['confidence_measure']);
    $this->setReportConfiguration($options);
  }

  /**
   * Gets the confidence_measure property.
   */
  public function getConfidenceMeasure() {
    return $this->confidence_measure;
  }

  /**
   * Returns the configuration for the reports.
   *
   * @return array
   *   An associative array that can have the following keys:
   *   - 'today_only' Boolean indicating whether reports should only be for the
   *     current day.
   *   - 'date_from' The start date for the reports
   *   - 'date_to' The end date for the reports
   *   - 'decision_name' The specific decision the report is for
   *   - 'machine_name' The agent machine name
   *   - 'confidence_measure' The confidence measure to use in the reports
   *   - 'goal' A particular goal to restrict reports to
   *   -  conversion_metric: Metric to display within the conversion report. One
   *      of 'rate' (conversion rate) or 'value' (conversion value).
   */
  protected function getReportConfiguration() {
    return $this->reportConfig;
  }

  /**
   * Converts the passed in options to configuration for all reports.
   *
   * @param $options
   *   An array of options for the report.
   *   - decision: (Optional) decision point name to limit results.
   *   - start: (Optional) start date for report, defaults to agent start.
   *   - end: (Optional) end date for report, defaults to current date.
   *   - goal: (Optional) goal to show in report, defaults to all.
   *   - conversion_metric: (Optional) metric to show in report, defaults to
   *     'rate'.
   */
  protected function setReportConfiguration($options) {
    // Set the loadedReports property to NULL because the configuration might
    // have changed.
    $this->loadedReports = NULL;
    $decision_name = empty($options['decision']) ? NULL : $options['decision'];
    $date_from = empty($options['start']) ? NULL : $options['start'];
    $date_to = empty($options['end']) ? NULL : $options['end'];

    $machine_name = $this->agent->getMachineName();
    // Campaign start and end time could be different from the report dates and
    // are used for the "total time running" info.
    $start_time = $this->agent->getStartTime();
    $end_time = time();
    $date_from = empty($date_from) ? date('Y-m-d', $start_time) : $date_from;
    $date_to = empty($date_to) ? date('Y-m-d', $end_time) : $date_to;

    $confidence_measure = $this->getConfidenceMeasure();
    // Convert the confidence measure from a percentage to a value between
    // 0 and 1 as expected by the Lift API.
    $confidence_measure /= 100;

    $agent_data = $this->agent->getData();
    // Set the configuration for the reports.
    $this->reportConfig = array(
      'test_type' => $agent_data['decision_style'] === 'adaptive' ? t('Auto-personalize') : t('A/B'),
      'campaign_start' => $start_time,
      'date_from' => $date_from,
      'date_to' => $date_to,
      'decision_name' => $decision_name,
      'machine_name' => $machine_name,
      'confidence_measure' => $confidence_measure,
      'goal' => empty($options['goal']) ? NULL : $options['goal'],
      'conversion_metric' => empty($options['conversion_metric']) ? 'rate' : $options['conversion_metric']
    );
  }

  /**
   * Given reporting data, returns the markup for all campaign reports.
   *
   * @param int $session_count
   *   The total number of sessions.
   * @param int $goal_count
   *   The total number of goals received.
   * @param array $daily_data
   *   The per-variation results broken down by day (shown in the graph)
   * @param array $aggregated_data
   *   The per-variation results aggregated across all dates
   * @param bool $confidence
   *   Boolean indicating whether there is overall confidence
   * @param int $winner
   *   Index of the winning variation, or NULL if there isn't one.
   */
  public function buildCampaignReports($session_count, $goal_count, $daily_data, $aggregated_data, $confidence, $winner) {
    $config = $this->getReportConfiguration();
    $from = isset($config['campaign_start']) ?  date('Y-m-d', $config['campaign_start']) : $config['date_from'];
    $to = isset($config['campaign_completed']) ?  date('Y-m-d', $config['campaign_completed']) : $config['date_to'];
    $interval = date_diff(date_create($from), date_create($to));
    $name = empty($config['goal']) ? t('All goals') : $config['goal'];
    $reports = array(
      'overview' => $this->buildOverviewReport($interval, $config['test_type'], $session_count, $goal_count),
      'experiment' => $this->buildExperimentReports($name, $daily_data, $aggregated_data, $confidence, $winner),
    );
    $reports['#has_data'] = $this->hasData;
    return $reports;
  }

  /**
   * Returns a render array representing the overview report for the given dates.
   *
   * @param DateInterval $interval
   *   The length of the period this report represents, as a DateInterval object.
   * @param string $type
   *   THe type of personalization, e.g. 'A/B test'
   * @param int $session_count
   *   The total number of sessions.
   * @param int $goal_count
   *   The total number of goals received.
   * @return array
   *   A render array representing the overview report.
   */
  protected function buildOverviewReport(DateInterval $interval, $type, $session_count, $goal_count) {
    // Create report renderable.
    $build = array();
    $build['test_type'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#title' => $type,
      '#description' => t('test type'),
      '#attributes' => array(
        'id' => 'acquia-lift-overview-type',
      )
    );
    $build['total_running'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-running',
      ),
      '#title' =>  $interval->format('%mm, %dd'),
      '#description' => t('total time running'),
    );
    $formatted_session_count = $this->formatReportNumber($session_count, FALSE, 0);
    $build['shown'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-shown',
      ),
      '#title' => $formatted_session_count,
      '#description' => format_plural($session_count, 'time shown', 'times shown'),
    );
    $formatted_goal_count = $this->formatReportNumber($goal_count, FALSE, 0);
    $build['goals'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-goals',
      ),
      '#title' => $formatted_goal_count,
      '#description' => t('goals met'),
    );
    if ($goal_count > 0) {
      $build['goals']['#attributes']['class'] = array('acquia-lift-report-positive');
    }
    return $build;
  }

  /**
   * Returns a render array for the experimental (confidence) reports.
   *
   * @param string $title
   *   The title of the report.
   * @param array $daily_data
   *   The per-variation results broken down by day (shown in the graph)
   * @param array $aggregated_data
   *   The per-variation results aggregated across all dates
   * @param bool $confidence
   *   Boolean indicating whether there is overall confidence
   * @param int $winner
   *   Index of the winning variation, or NULL if there isn't one.
   */
  protected function buildExperimentReports($title, $daily_data, $aggregated_data, $confidence, $winner) {
    $build = array();
    $build['reports'] = array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('lift-statistics'),
      ),
      'conversion' => array(),
    );
    $build['reports']['conversion']['title'] = array(
      '#theme' => 'html_tag',
      '#tag' => 'h3',
      '#value' => $title,
      '#attributes' => array('class' => array('lift-statistic-category-name', 'element-invisible')),
    );
    $build['reports']['conversion']['detail'] = $this->buildConversionDetailReport($daily_data);
    $build['reports']['conversion']['summary'] = $this->buildConversionSummaryReport($aggregated_data, $confidence, $winner);
    $build['reports']['conversion']['#theme_wrappers'] = array('container');
    $build['reports']['conversion']['#attributes'] = array('class' => array('lift-statistic-category'));
    return $build;
  }

  /**
   * Builds the render array for the metrics portion of the report.
   *
   * @param array $daily_data
   *   The per-variation results broken down by day (shown in the graph)
   * @param array
   *   A render array for the report.
   */
  protected function buildConversionDetailReport($daily_data) {
    $config = $this->getReportConfiguration();
    $headers = array(
      t('Date'),
      t('Content variation'),
      array(
        'data' => t('Conversion rate (%)'),
        'data-conversion-metric' => 'rate',
      ),
      array(
        'data' => t('Conversion value'),
        'data-conversion-metric' => 'value',
      ),
      t('Margin of error'),
    );
    $build = $rows = array();
    foreach ($daily_data as $choice_data) {
      $rows[] = array(
        'data' => array(
          array(
            'data' => $choice_data['timestamp'],
          ),
          array(
            'data' => $choice_data['option_label'],
            'data-acquia-lift-variation-label' => $this->getVariationLabel($choice_data['counter'], $choice_data['control']),
          ),
          array(
            'data' => $choice_data['conversion'],
          ),
          array(
            'data' => $choice_data['conversion_value'],
          ),
          array(
            'data' => $choice_data['margin_error'],
          ),
        ),
        'no_striping' => TRUE,
      );
    }
    if (!empty($rows)) {
      $build['metric_table'] = array(
        '#theme' => 'table',
        '#header' => $headers,
        '#rows' => $rows,
        '#sticky' => FALSE,
        '#attributes' => array(
          'data-lift-statistics' => '',
          'data-liftGraph-columnName' => '2',
          'data-liftGraph-columnX' => '1',
          'data-liftGraph-renderer' => 'line',
          'data-liftgraph-excluded' => '5',
          'data-acquia-lift-campaign' => $config['machine_name'],
          'data-acquia-lift-decision-name' => $config['decision_name'],
        )
      );
    }
    return $build;
  }

  /**
   * Builds the render array for the summary portion of the report.
   *
   * @param array $aggregated_data
   *   The per-variation results aggregated across all dates
   * @param bool $confidence
   *   Whether the report has confidence.
   * @param bool $winner
   *   The index of the winning option or NULL if there's no winner
   * @param array
   *   A render array for the report.
   */
  protected function buildConversionSummaryReport($aggregated_data, $confidence, $winner) {
    $config = $this->getReportConfiguration();
    $headers = array(
      t('Variation'),
      array(
        'data' => t('Times shown'),
        'data-help-tooltip' => t('Number of times this variation was shown to the experimental group.'),
      ),
      array(
        'data' => t('Goals met'),
        'data-help-tooltip' => t('Number of times visitors in the experiement group completed a goal after viewing the variation.'),
      ),
      array(
        'data' => t('Conversion rate'),
        'data-help-tooltip' => t('Percentage of goals met for each display of the variation.'),
      ),
      array(
        'data' => t('Chance to beat control'),
        'data-help-tooltip' => t('Confidence that this variation will perform better than the control.'),
      ),
      array(
        'data' => t('Lift'),
        'data-help-tooltip' => t('Estimated increase in conversions if this variation were to be shown all the time, versus the control.'),
      ),
      array(
        'data' => t('Winner'),
        'data-help-tooltip' => t('Most effective variation for visitors based on a @confidence% confidence level.', array(
          '@confidence' => $this->getConfidenceMeasure(),
        )),
      ),
    );
    $confidence_message_shown = FALSE;
    $rows = array();
    $num_rows = count($aggregated_data);
    foreach ($aggregated_data as $choice_id => $choice_data) {
      $row_data = array(
        array(
          'data' => $choice_data['option_label'],
          'data-acquia-lift-variation-label' => $this->getVariationLabel($choice_data['counter'], $choice_data['control']),
        ),
        array(
          'data' => $choice_data['count'],
        ),
        array(
          'data' => $choice_data['goals'],
        ),
        array(
          'data' => $choice_data['conversion'],
        ),
        array(
          'data' => $choice_data['confidence'],
        ),
        array(
          'data' => $choice_data['lift_default'],
        ),
      );
      // Add the winner column data.
      if (empty($rows) && !$confidence) {
        // If there is low confidence then show the message throughout the
        // winner column.
        $row_data[] = array(
          'data' => $this->getLowConfidenceMessage(),
          'rowspan' => $num_rows,
          'class' => array('acquia-lift-ab-winner'),
        );
        $confidence_message_shown = TRUE;
      }
      else if (!$confidence_message_shown) {
        // Show the winner indicator if this is the winning variation.
        $row_data[] = $confidence && ($winner === $choice_data['counter']) ? '<span class="lift-winner">' . t('Winner') . '</span>' : '';
      }
      $rows[] = array(
        'data' => $row_data,
        'no_striping' => TRUE,
      );

    }
    if (empty($rows)) {
      return array();
    }
    $build['summary_holder'] = array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('lift-graph-result'),
      ),
    );
    $build['summary_holder']['summary_table'] = array(
      '#theme' => 'table',
      '#header' => $headers,
      '#rows' => $rows,
      '#sticky' => FALSE,
      '#attributes' => array(
        'class' => array('lift-graph-result-data'),
        'data-acquia-lift-campaign' => $config['machine_name'],
        'data-acquia-lift-decision-name' => $config['decision_name'],
      ),
      '#attached' => array(
        'library' => array(
          array('acquia_lift', 'acquia_lift.help'),
        ),
      ),
    );
    return $build;
  }


  /**
   * Generates a message to show when there is insufficient confidence in the
   * test results.
   *
   * @return string
   */
  protected function getLowConfidenceMessage() {
    return t('There is not enough data to declare a winner with @confidence% confidence. Consider letting the test run longer before using the results.', array(
      '@confidence' => $this->getConfidenceMeasure(),
    ));
  }

  /**
   * Generates the variation abbreviated label.
   *
   * @param $counter
   *   Indicates the number for the variation.
   * @param $is_control
   *   True if this is the control option.
   */
  protected function getVariationLabel($counter, $is_control) {
    if ($is_control) {
      return t('Control');
    }
    else {
      return t('V@num', array('@num' => $counter));
    }
  }

  /**
   * Formats a percentage value for use in reports.
   *
   * @param $value
   *   The number to show as a percentage.
   * @param bool $include_sign
   *   True to include positive/negative sign indicators.
   * @param $trim
   *   Boolean indicating whether the number should be trimmed of trailing 0s.
   * @param $decimals
   *   The number of decimal places to display.
   * @param $padding
   *   The total number of characters (including decimal) for padding of the
   *   final number.  This allows numbers to align properly  in column views.
   *   This will have no effect if trim is set to true.
   * @return string
   *   The formatted number to display.
   */
  protected function formatReportPercentage($value, $include_sign = FALSE, $trim = TRUE, $decimals = 2, $padding = 1) {
    $percent = (float) $value * 100;
    if ($percent > 0 && $include_sign) {
      return '+' . $this->formatReportNumber($percent, $trim, $decimals, $padding) . '%';
    }
    return $this->formatReportNumber($percent, $trim, $decimals, $padding) . '%';
  }

  /**
   * Formats a number value for use in reports.
   *
   * @param $value
   *   The number of format (or an empty value).
   * @param $trim
   *   Boolean indicating whether the number should be trimmed of trailing 0s.
   * @param $decimals
   *   The number of decimal places to display.
   * @param $padding
   *   The total number of characters to pad to the left of the decimal point.
   * @return string
   *   The formatted number to display.
   */
  protected function formatReportNumber($value, $trim = TRUE, $decimals = 2, $padding = 1) {
    if (is_numeric($value)) {
      $value = number_format($value, $decimals);
      if ($trim && strpos('.', $value) !== FALSE) {
        $value = rtrim(rtrim($value, '0'), '.');
      }
      if ($padding > 0) {
        $value = str_pad($value, $padding, '0', STR_PAD_LEFT);
      }
    }
    if (empty($value)) {
      $value = 0;

    }
    return $value;
  }

}

