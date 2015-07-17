<?php
/**
 * @file
 * Provides a report class for legacy agents.
 */

/**
 * Base class providing report data loading functionality common to all
 * Acquia Lift Reports.
 */
class AcquiaLiftReportLegacy {

  /**
   * The value to show when report data is not applicable.
   */
  const DATA_NA = '&mdash;';

  /**
   * The value representing no features applied to an experiment.
   */
  const NO_FEATURES = '(none)';


  /**
   * The threshold to use above which lift percentage will be positively noted.
   */
  const LIFT_THRESHHOLD = 0;

  /**
   * The threshold to use above which stability will be positively noted.
   */
  const STABILITY_THRESHOLD = 25;

  /**
   * The Acquia Lift agent instance for reporting on.
   *
   * @var AcquiaLiftAgent
   */
  protected $agent;

  /**
   * An instance of AcquiaLiftAPI.
   *
   * @var AcquiaLiftAPI
   */
  protected $liftAPI;

  /**
   * The confidence measure for determining statistical significance.
   */
  protected $confidence_measure = 95;

  /**
   * The extracted report data for each of the Acquia Lift API calls keyed
   * by date/feature set requested.
   *
   * @var array;
   */
  protected $report_data;

  /**
   * Constructs an AcquiaLiftReport object
   *
   * @param PersonalizeAgentInterface $agent
   *   The agent the report is for.
   *
   * @param AcquiaLiftReportDataSourceInterface $report_data_src
   *   The source for the report data.
   */
  function __construct(PersonalizeAgentInterface $agent, $options = array(), AcquiaLiftReportDataSourceInterface $report_data_src) {
    $this->agent = $agent;
    $this->reportDataSrc = $report_data_src;
    $this->options = $options;
    $this->setConfidenceMeasure($options['confidence_measure']);
  }

  /**
   * Gets the confidence_measure property.
   */
  public function getConfidenceMeasure() {
    return $this->confidence_measure;
  }

  /**
   * Sets the confidence_measure property.
   */
  public function setConfidenceMeasure($value) {
    if ($value < 0) {
      $value = 0;
    }
    if ($value > 100) {
      $value = 100;
    }
    $this->confidence_measure = $value;
  }

  /**
   * Build the conversion report.
   */
  public function buildConversionReport($options) {
    $report_data = $this->generateReportConfiguration($options);
    $report_name = t('All goals');

    $report_options = array();
    if (!empty($options['goal'])) {
      // Get the summary data for the whole campaign in order to have overall
      // statistics.
      $this->loadConversionReportHelper($report_data, FALSE, $options);
      $report_options['goal'] = $options['gaol'];
      $actions = visitor_actions_get_actions();
      // If the action still exists, use the label, otherwise just use the goal
      // machine name.
      if (isset($actions[$options['goal']])) {
        $report_name = $actions[$options['goal']]['label'];
      }
      else {
        $report_name = $options['goal'];
      }

    }
    $this->loadConversionReportHelper($report_data, TRUE, $report_options);
    $this->loadConversionReportHelper($report_data, FALSE, $report_options);

    if ($report_data['conversion_all'] == FALSE) {
      return array(
        '#markup' => t('There was a problem generating the conversion report.'),
      );
    }

    $reports = $this->buildConversionReports(array(
      'name' => $report_name,
      'detail' => empty($goal) ? $report_data['conversion_all']['detail'] : $report_data['conversion_goals'][$goal]['detail'],
      'summary' => empty($goal) ? $report_data['conversion_all']['summary'] : $report_data['conversion_goals'][$goal]['summary'],
    ), $report_data);
    return $reports;
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
   * Generates an internal raw report name for a confidence report based on
   * the options.
   *
   * @param $options
   *   An array of options passed to the confidence report loader.
   * @returns string
   *   The name of a report to reference within the raw reporting data.
   */
  protected function getConfidenceReportRawName($options = array()) {
    $report_name = 'confidence';
    if (!empty($options['goal'])) {
      $report_name .= '_' . $options['goal'];
    }
    if (isset($options['aggregated-over-dates']) && $options['aggregated-over-dates'] == FALSE) {
      $report_name .= '_detail';
    }
    return $report_name;
  }

  /**
   * Helper function to generate the report options necessary to get a detailed
   * confidence report rather than a summary report.
   *
   * @returns array
   *   An array of confidence report options.
   */
  protected function getConfidenceDetailReportOptions() {
    $options['aggregated-over-dates'] = FALSE;
    return $options;
  }

  /**
   * Generates the general report configuration that is used to load any report.
   *
   * @param $options
   *   An array of options for the report.
   *   - decision: (Optional) decision point name to limit results.
   *   - start: (Optional) start date for report, defaults to agent start.
   *   - end: (Optional) end date for report, defaults to current date.
   *   - goal: (Optional) goal to show in report, defaults to all.
   *   - conversion_metric: (Optional) metric to show in report, defaults to
   *     'rate'.
   * @return array
   *   The basic reporting configuration.
   */
  protected function generateReportConfiguration($options) {
    $decision_name = empty($options['decision']) ? NULL : $options['decision'];
    $date_from = empty($options['start']) ? NULL : $options['start'];
    $date_to = empty($options['end']) ? NULL : $options['end'];

    $machine_name = $this->agent->getMachineName();
    $today_only = $date_from === date('Y-m-d') && empty($date_to);
    $date_from = empty($date_from) ? date('Y-m-d', $this->agent->getStartTime()) : $date_from;
    $date_to = empty($date_to) ? date('Y-m-d') : $date_to;
    $key = 'S' . $date_from . 'E' . $date_to;

    if (!isset($this->report_data[$key])) {
      $confidence_measure = $this->getConfidenceMeasure();
      // Convert the confidence measure from a percentage to a value between
      // 0 and 1 as expected by the Lift API.
      $confidence_measure /= 100;

      // Save basic report generation information with the data.
      $this->report_data[$key]['today_only'] = $today_only;
      $this->report_data[$key]['date_from'] = $date_from;
      $this->report_data[$key]['date_to'] = $date_to;
      $this->report_data[$key]['decision_name'] = $decision_name;
      $this->report_data[$key]['machine_name'] = $machine_name;
      $this->report_data[$key]['confidence_measure'] = $confidence_measure;
      $this->report_data[$key]['features'] = array(self::NO_FEATURES);
      $this->report_data[$key]['goal'] = empty($options['goal']) ? NULL : $options['goal'];
      $this->report_data[$key]['conversion_metric'] = empty($options['conversion_metric']) ? 'rate' : $options['conversion_metric'];
    }
    return $this->report_data[$key];
  }


  /**
   * Loads the context filter raw values into the report data.
   *
   * @param $report_data
   *   The current reporting array by reference.
   * @return array
   *   The updated reporting data for chaining purposes.  Note that the
   *   reporting data is updated by reference as well.
   */
  protected function loadContextFilterData(&$report_data) {
    // Check and see if it is already loaded.
    if (isset($report_data['raw']['potential_context'])) {
      return $report_data;
    }
    // Load it from the report source.
    try {
      $report_data['raw']['potential_context'] = $this->getContextFilters($report_data['machine_name']);
    }
    catch (Exception $e) {
      $report_data['raw']['potential_context']['error'] = $e->getMessage();
    }
    return $report_data;
  }


  /**
   * Loads the agent status raw reporting data.
   *
   * @param $report_data
   *   The current reporting array by reference.
   * @return array
   *   The updated reporting data for chaining purposes.  Note that the
   *   reporting data is updated by reference as well.
   */
  protected function loadAgentStatusData(&$report_data) {
    // Check and see if it is already loaded.
    if (isset($report_data['raw']['status'])) {
      return $report_data;
    }
    try {
      if ($report_data['today_only']) {
        $num_days = 1;
      }
      else {
        $interval = date_diff(date_create($report_data['date_from']), date_create($report_data['date_to']));
        $num_days = $interval->days;
      }
      $report_data['raw']['status'] = $this->reportDataSrc->getAgentStatusReport(array($report_data['machine_name']), $num_days);
    }
    catch (Exception $e) {
      $report_data['raw']['status']['error'] = $e->getMessage();
    }
    return $report_data;
  }

  /**
   * Loads the agent confidence raw reporting data.
   *
   * @param $report_data
   *   The current reporting array by reference.
   * @param $options
   *   (Optional) An array of options to pass to the report source.
   * @return array
   *   The updated reporting data for chaining purposes.  Note that the
   *   reporting data is updated by reference as well.
   */
  protected function loadConfidenceData(&$report_data, $options = array()) {
    $report_name = $this->getConfidenceReportRawName($options);
    // Check and see if it is already loaded.
    if (isset($report_data['raw'][$report_name])) {
      return $report_data;
    }
    try {
      $defaults = array(
        'features' => 'all',
        'confidence-measure' => $report_data['confidence_measure'],
      );
      $options = array_merge($defaults, $options);
      $report_data['raw'][$report_name] = $this->reportDataSrc->getConfidenceReport(
        $report_data['machine_name'],
        $report_data['date_from'],
        $report_data['date_to'],
        $report_data['decision_name'],
        $options
      );
    }
    catch (Exception $e) {
      $report_data['raw'][$report_name]['error'] = $e->getMessage();
    }
    return $report_data;
  }

  /**
   * Duplication of the getDateString in the API class.
   */
  protected static function getDateString($date_start, $date_end) {
    if ($date_start === NULL || !preg_match('/\d{4}\-\d{2}\-\d{2}/', $date_start)) {
      $date_start = date('Y-m-d');
    }
    $date_str = '/' . $date_start;
    if ($date_end !== NULL && preg_match('/\d{4}\-\d{2}\-\d{2}/', $date_end)) {
      $date_str .= '/' . $date_end;
    }
    return $date_str;
  }

  /**
   * Loads the agent targeting raw reporting data.
   *
   * @param $report_data
   *   The current reporting array by reference.
   * @return array
   *   The updated reporting data for chaining purposes.  Note that the
   *   reporting data is updated by reference as well.
   */
  protected function loadTargetingData(&$report_data) {
    // Check and see if it is already loaded.
    if (isset($report_data['raw']['targeting'])) {
      return $report_data;
    }
    try {
      if ($this->reportDataSrc instanceof AcquiaLiftReportDataFromFile) {
        $report_data['raw']['targeting'] = $this->reportDataSrc->getTargetingImpactReport(
          $report_data['machine_name'],
          $report_data['date_from'],
          $report_data['date_to'],
          $report_data['decision_name']
        );
      }
      elseif ($this->reportDataSrc instanceof AcquiaLiftAPI) {
        $agent_name = $this->agent->getMachineName();
        $date_str = self::getDateString($report_data['date_from'], $report_data['date_to']);
        $path = "/{$agent_name}/report/targeting-features{$date_str}";
        $report_data['raw']['targeting'] = $this->reportDataSrc->makeLegacyGetRequest($path, array(), 'Problem retrieving targeting impact report.');
      }
    }
    catch (Exception $e) {
      $report_data['raw']['confidence']['error'] = $e->getMessage();
    }
    return $report_data;
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
      if ($trim) {
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

  /**
   * Builds the conversion reports to show basic conversion metrics for report
   * requested in the report_data.
   *
   * @param array $report_data
   *   The loaded report data for the selection decision and dates.
   * @return array
   *   The render array for the report.
   */
  protected function buildAllConversionReports($report_data) {
    if (empty($report_data['goal'])) {
      // Generate the conversion reports for all goals.
      $reports = $this->buildConversionReports(array(
        'name' => t('All goals'),
        'detail' => $report_data['conversion_all']['detail'],
        'summary' => $report_data['conversion_all']['summary'],
      ), $report_data);
    }
    else {
      // Generate the conversion reports for the specified goal.
      $reports = $this->buildConversionReports($report_data['conversion_goals'][$report_data['goal']], $report_data);
    }
    if ($reports == FALSE) {
      drupal_set_message(t('There was a problem retrieving the report data.  Please try again later.'), 'error');
    }
    $build['reports'] = array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('lift-statistics'),
      ),
      'conversion' => $reports,
    );
    return $build;
  }

  /**
   * Handles all of the logic to load and extract a conversion report.
   *
   * @param $report_data
   *   The array of existing report data.
   * @param $detail
   *   True if the report should be a detailed report and false for summary.
   * @param array $options
   *   An array of report options such as a goal for limiting.
   */
  protected function loadConversionReportHelper(&$report_data, $detail, $options = array()) {
    // Limit report results to the experimental group.
    $options['policies'] = 'explore';
    if ($detail) {
      $options = array_merge($options, $this->getConfidenceDetailReportOptions());
    }
    $new_report = array();
    $report_name = $this->getConfidenceReportRawName($options);
    if (isset($options['goal'])) {
      // Add the report name for this goal to track which goals are included.
      $report_data['goal_reports'][] = $report_name;
      // Determine the parent for reports added by this helper.
      if (!isset($report_data['conversion_goals'])) {
        $report_data['conversion_goals'][$options['goal']] = FALSE;
      }
      $detail_report = &$report_data['conversion_goals'][$options['goal']];
    }
    else {
      // Determine the parent for reports added by this helper.
      if (!isset($report_data['conversion_all'])) {
        $report_data['conversion_all'] = FALSE;
      }
      $detail_report = &$report_data['conversion_all'];
    }
    // Check if the report is already loaded.
    $subreport_name = $detail ? 'detail' : 'summary';
    if (isset($detail_report[$subreport_name])) {
      return;
    }
    // Load the raw report data.
    if (!isset($report_data['raw'][$report_name])) {
      $this->loadConfidenceData($report_data, $options);
      if (isset($report_data['raw'][$report_name]['error'])) {
        return;
      }
    }
    // Extract the data into full report data from the raw data.
    $detail_report[$subreport_name] = $detail ? $this->extractConversionReportData($report_data['raw'][$report_name]['data']['items']) : $this->extractConversionSummaryData($report_data['raw'][$report_name]['data']['items']);
  }

  /**
   * Extracts the required overview data from the report data returned by
   * Acquia Lift.
   *
   * @param $items
   *   An array of items as returned from Acquia Lift.
   * @return array
   *   An associative array with information for today's and the total overview.
   */
  protected function extractOverviewReportData($items) {
    $agent_data = $this->agent->getData();
    $decision_style = $agent_data['decision_style'] === 'adaptive' ? t('Auto-personalize') : t('A/B');
    $total_variations = isset($agent_data['decisions']) ? count($agent_data['decisions']) : 0;

    // Create an array of data for each time option.
    $report['today'] = array(
      'unformatted' => array(
        'total_lift' => $items['today']['liftOverDefaultUsingGoals'],
        'total_shown' => $items['today']['sessionCount'],
        'total_goals' => $items['today']['goalCount'],
      ),
      'test_type' => $decision_style,
      'total_shown' => $this->formatReportNumber($items['today']['sessionCount']),
      'total_goals' => $this->formatReportNumber($items['today']['goalCount']),
      'total_goals_positive' => $items['today']['goalCount'] > 0,
      'total_lift' => $this->formatReportPercentage($items['today']['liftOverDefaultUsingGoals']),
      'total_variations' => $total_variations,
    );
    $report['all'] = array(
      'unformatted' => array(
        'total_shown' => $items['totals']['sessions']['count'],
        'total_goals' => $items['totals']['goals']['count'],
        'total_lift' => $items['today']['liftOverDefaultUsingGoalsToDate'],
      ),
      'test_type' => $decision_style,
      'total_shown' => $this->formatReportNumber($items['totals']['sessions']['count']),
      'total_goals' => $this->formatReportNumber($items['totals']['goals']['count']),
      'total_goals_positive' => $items['totals']['goals']['count'] > 0,
      'total_lift' => $this->formatReportPercentage($items['today']['liftOverDefaultUsingGoalsToDate']),
      'total_variations' => $total_variations,
    );
    return $report;
  }

  /**
   * Extracts data from the raw confidence detail report that is prepared for use
   * within the conversion report rendering process.
   *
   * @param $items
   *   An array of items as return from Acquia Lift.
   * @return array
   *   An associative array with information about the performance of each choice.
   */
  protected function extractConversionReportData($items) {
    if (empty($items)) {
      return array();
    }
    $shown = array();
    $counter = NAN;
    $data = array();

    $total_count = $total_goals = $total_val = array();
    foreach ($items as $item) {
      // Check to see if we are in a new grouping of choices.
      $check = $item['choice'];
      if (is_nan($counter) || isset($shown[$check])) {
        $shown = array();
        $counter = 0;
      }
      else {
        $counter++;
      }
      $shown[$check] = $check;

      $choice = $option_id = $item['choice'];
      $choice_id = $choice;
      if (strpos($choice, ':') !== FALSE) {
        list($decision_name, $option_id) = explode(':', $choice);
      }
      if ($option_label = personalize_get_option_label_for_decision_and_choice($decision_name, $option_id)) {
        $choice_id = $option_label;
      }
      $goals = $item['totals']['goals'];
      $count = $item['totals']['count'];
      $val = $item['totals']['val'];
      $total_count[$counter] = isset($total_count[$counter]) ? $total_count[$counter] + $count : $count;
      $total_goals[$counter] = isset($total_goals[$counter]) ? $total_goals[$counter] + $goals : $goals;
      $total_val[$counter] = isset($total_val[$counter]) ? $total_val[$counter] + $val : $val;

      $rate = $total_count[$counter] > 0 ? ($total_goals[$counter]/$total_count[$counter]) * 100 : 0;
      $val_rate = $total_count[$counter] > 0 ? ($total_val[$counter]/$total_count[$counter]) : 0;
      $margin = (($item['bHi'] - $item['bLo'])/2) * 100;

      $data[$item['feature']][] = array(
        'choice_id' => $choice_id,
        'raw_label' => $option_id,
        'goals' => $total_goals[$counter],
        'count' => $total_count[$counter],
        'date' => $item['date'],
        'timestamp' => strtotime($item['date']),
        'conversion' => $this->formatReportNumber($rate, TRUE, 4),
        'conversion_value' => $this->formatReportNumber($val_rate, TRUE, 4),
        'estimated_value' => $this->formatReportNumber($item['vMean'], TRUE, 4),
        'margin_error' => $this->formatReportNumber($margin, TRUE, 4),
        'counter' => $counter,
        'control' => $counter === 0,
      );
    }
    return $data;
  }

  /**
   * Loads and formats the necessary reporting data in order to generate a
   * conversion metrics graph/report.
   *
   * @param array
   *   The report data object to load conversion report data into.
   */
  protected function loadConversionReportData(&$report_data) {
    // All goals - summary report is always loaded in order to get the overview
    // data for the campaign.
    if (!isset($report_data['conversion'])) {
      $this->loadConversionReportHelper($report_data, FALSE);
    }
    // All goals conversion report.
    if (empty($report_data['goal'])) {
      // All goals - detail report.
      if (!isset($report_data['conversion_detail'])) {
        $this->loadConversionReportHelper($report_data, TRUE);
      }
      return;
    }
    // Load the detail and summary reports for the specified goal.
    $actions = visitor_actions_get_actions();
    $goal_id = $report_data['goal'];
    if (!isset($report_data['conversion_goals'][$goal_id])) {
      $report_data['conversion_goals'][$goal_id]['name'] = isset($actions[$goal_id]) ? $actions[$goal_id]['label'] : $goal_id;
      $options['goal'] = $goal_id;
      // Summary report.
      $this->loadConversionReportHelper($report_data, FALSE, $options);
      // Detail report.
      $this->loadConversionReportHelper($report_data, TRUE, $options);
    }
  }

  /**
   * Extracts data from the raw aggregate confidence report that is prepared for
   * use within the report rendering process.
   *
   * @param $items
   *   An array of items as return from Acquia Lift.
   * @return array
   *   An associative array with information about the performance of each choice.
   */
  protected function extractConversionSummaryData($items) {
    if (empty($items)) {
      return array();
    }
    $shown = array();
    $counter = NAN;
    $data = array();
    $confidence = FALSE;
    $winner = '';
    $winning_value = NAN;
    foreach ($items as $item) {
      // Check to see if we are in a new grouping of choices.
      $check = $item['choice'];
      // First item in the loop so this is the control.
      if (is_nan($counter) || isset($shown[$check])) {
        $shown = array();
        $counter = 0;
      }
      // One of the non-control variations.
      else {
        $counter++;
      }
      $shown[$check] = $check;

      $choice = $option_id = $item['choice'];
      $choice_id = $choice;
      if (strpos($choice, ':') !== FALSE) {
        list($decision_name, $option_id) = explode(':', $choice);
      }
      if ($option_label = personalize_get_option_label_for_decision_and_choice($decision_name, $option_id)) {
        $choice_id = $option_label;
      }
      $goals = $item['totals']['goals'];
      $count = $item['totals']['count'];
      // The format for percentages will already multiply by 100.
      $rate = $count > 0 ? ($goals/$count) : 0;
      $margin = (($item['bHi'] - $item['bLo'])/2);

      if ($item['signif']) {
        if (is_nan($winning_value) || $item['confidence'] > $winning_value) {
          $winning_value = $item['lift']['default'];
          $winner = $counter;
          $confidence = TRUE;
        }
      }

      $data[$item['feature']][] = array(
        'counter' => $counter,
        'choice_id' => $choice_id,
        'raw_label' => $option_id,
        'goals' => $goals,
        'count' => $count,
        'date' => $item['date'],
        'timestamp' => strtotime($item['date']),
        'conversion' => $this->formatReportPercentage($rate),
        'estimated_value' => $this->formatReportNumber($item['vMean'], TRUE, 4),
        'estimated_higher' => $this->formatReportNumber($item['bHi'], TRUE, 4),
        'estimated_lower' => $this->formatReportNumber($item['bLo'], TRUE, 4),
        'margin_error' => $this->formatReportNumber($margin, TRUE, 4),
        'significant' => $item['signif'],
        'control' => $counter === 0,
        'confidence' => $counter === 0 ? self::DATA_NA : $this->formatReportPercentage($item['confidence']/100),
        'lift_default' => $counter === 0 ? self::DATA_NA : $this->formatReportPercentage($item['lift']['default']/100, TRUE),
        'lift_random' => $this->formatReportPercentage($item['lift']['random']/100, TRUE),
      );
    }
    $report = array(
      'data' => $data,
      'overview' => array(
        'confidence' => $confidence,
        'winner' => $winner,
      ),
    );
    return $report;
  }

  /**
   * Builds the render array for the metrics portion of the report.
   *
   * @param array $report_data
   *   All of the reporting data for this AB report.
   * @param array $all_report_data
   *   The report data for all reports to be build including overview data.
   * @param array|bool
   *   A render array for the report or FALSE if it cannot be generated.
   */
  protected function buildConversionDetailReport($report_data, $all_report_data) {
    if ($report_data === FALSE) {
      return FALSE;
    }
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
      t('Margin of error (%)'),
    );
    $rows = array();
    foreach ($report_data as $feature => $feature_data) {
      if (!in_array($feature, $all_report_data['features'])) {
        continue;
      }
      foreach($feature_data as $data) {
        $rows[] = array(
          'data' => array(
            array(
              'data' => $data['timestamp'],
            ),
            array(
              'data' => $data['choice_id'],
              'data-acquia-lift-variation-label' => $this->getVariationLabel($data['counter'], $data['control']),
            ),
            array(
              'data' => $data['conversion'],
            ),
            array(
              'data' => $data['conversion_value'],
            ),
            array(
              'data' => $data['margin_error'],
            ),
          ),
          'no_striping' => TRUE,
        );
      }
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
          'data-acquia-lift-campaign' => $all_report_data['machine_name'],
          'data-acquia-lift-decision-name' => $all_report_data['decision_name'],
        )
      );
    }
    return $build;
  }

  /**
   * Builds the render array for the summary portion of the report.
   *
   * @param array $report_data
   *   Reporting data for this summary report.
   * @param array $all_report_data
   *   The report data for all reports to be build including overview data.
   * @param array|bool
   *   A render array for the report or FALSE if it cannot be generated.
   */
  protected function buildConversionSummaryReport($report_data, $all_report_data) {
    if ($report_data === FALSE) {
      return FALSE;
    }
    $confidence = $all_report_data['conversion_all']['summary']['overview']['confidence'];
    $winner = $all_report_data['conversion_all']['summary']['overview']['winner'];
    $headers = array(
      t('Variation'),
      array(
        'data' => t('Total goals met'),
        'data-help-tooltip' => t('Number of times visitors completed a goal after viewing the variation.'),
      ),
      array(
        'data' => t('Total conversion rate'),
        'data-help-tooltip' => t('Percentage of goals met for each display of the variation.'),
      ),
      array(
        'data' => t('Chance to beat control'),
        'data-help-tooltip' => t('Likelihood visitors will complete goals for a variation compared to the control.'),
      ),
      array(
        'data' => t('Lift'),
        'data-help-tooltip' => t('Likelihood visitors will complete goals for a variation compared to the control.'),
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
    foreach ($report_data['data'] as $feature => $feature_data) {

      if (!in_array($feature, $all_report_data['features'])) {
        continue;
      }
      foreach ($feature_data as $data) {
        $row_data = array(
          array(
            'data' => $data['choice_id'],
            'data-acquia-lift-variation-label' => $this->getVariationLabel($data['counter'], $data['control']),
          ),
          array(
            'data' => $data['goals'],
          ),
          array(
            'data' => $data['conversion'],
          ),
          array(
            'data' => $data['confidence'],
          ),
          array(
            'data' => $data['lift_default'],
          ),
        );
        // Add the winner column data.
        if (empty($rows) && $report_data['overview']['confidence'] === FALSE) {
          // If there is low confidence then show the message throughout the
          // winner column.
          $row_data[] = array(
            'data' => $this->getLowConfidenceMessage(),
            'rowspan' => count($feature_data),
            'class' => array('acquia-lift-ab-winner'),
          );
          $confidence_message_shown = TRUE;
        }
        else if (!$confidence_message_shown) {
          // Show the winner indicator if this is the winning variation.
          $row_data[] = $confidence && $winner == $data['counter'] ? '<span class="lift-winner">' . t('Winner') . '</span>' : '';
        }
        $rows[] = array(
          'data' => $row_data,
          'no_striping' => TRUE,
        );
      }
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
        'data-acquia-lift-campaign' => $all_report_data['machine_name'],
        'data-acquia-lift-decision-name' => $all_report_data['decision_name'],
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
   * Build a set of confidence reports from the report data.
   *
   * @param array $report_data
   *   The extracted report data with the following keys:
   *   - name: The name of the section
   *   - detail:  The detail report data
   *   - summary: The summary report data
   * @param array $all_report_data
   *   The full report data for all reports including overview information.
   * @return array|bool
   *   The render array for the reports or false if invalid data.
   */
  protected function buildConversionReports($report_data, $all_report_data) {
    if ($report_data['detail'] == FALSE || $report_data['summary'] == FALSE) {
      return FALSE;
    }
    $build = array();
    $build['reports']['title'] = array(
      '#theme' => 'html_tag',
      '#tag' => 'h3',
      '#value' => $report_data['name'],
      '#attributes' => array('class' => array('lift-statistic-category-name', 'element-invisible')),
    );
    $build['reports']['detail'] = $this->buildConversionDetailReport($report_data['detail'], $all_report_data);
    $build['reports']['summary'] = $this->buildConversionSummaryReport($report_data['summary'], $all_report_data);
    $build['reports']['#theme_wrappers'] = array('container');
    $build['reports']['#attributes'] = array('class' => array('lift-statistic-category'));
    return $build;
  }

  public function getCampaignReports() {
    return $this->buildCampaignReports();
  }

  /**
   * Build the conversion report.
   */
  public function buildCampaignReports() {
    $report_data = $this->loadReportData($this->options);
    $reports = array(
      'overview' => $this->buildOverviewReport($report_data),
      'experiment' => $this->buildAllConversionReports($report_data),
      'context' => $this->buildContextReport($report_data),
      'stability' => $this->buildStabilityReport($report_data),
      'targeting' => $this->buildReportContextSelection($report_data),
    );
    $reports['#has_data'] = isset($reports['overview']['shown']['#title']) ? $reports['overview']['shown']['#title'] > 0 : FALSE;

    if (!is_array($report_data['status']) || !is_array($report_data['confidence']) || !is_array($report_data['targeting']) || !is_array($report_data['potential_context'])) {
      drupal_set_message(t('There was a problem retrieving the report data.  Please try again later.'), 'error');
    }
    else if ($reports['#has_data'] && $report_data['status']['all']['total_confident'] == 0) {
      drupal_set_message($this->getLowConfidenceMessage(), 'warning');
    }
    return $reports;
  }

  /**
   * Loads all of the data necessary to generate the reports for the agent.
   *
   * @param $options
   * An array of report filter options.
   * - decision: Preselected decision to display.
   * - start: The start date of the report.
   * - from: The end date of the report.
   * - goal: (optional) A selected goal for reporting; defaults to all.
   * - conversion_metric: (optional) Metric to display within the conversion report.  One
   *   of 'rate' (conversion rate) or 'value' (conversion value).
   * @return array
   *   The reporting data for the date range as an array with keys for
   *   - status: The general agent status report data
   *   - confidence: The confidence report data
   *   - targeting: The targeting report data
   */
  protected function loadReportData($options) {
    $report_data = $this->generateReportConfiguration($options);
    $this->loadConversionReportData($report_data);

    // Context filters.
    if (!isset($report_data['potential_context'])) {
      $this->loadContextFilterData($report_data);
      if (isset($report_data['raw']['potential_context']['error'])) {
        $report_data['potential_context'] = FALSE;
      }
      else {
        $report_data['potential_context'] = $this->extractPotentialTargetingValues($report_data['raw']['potential_context']['data']);
      }
    }
    // Agent status
    if (!isset($report_data['status'])) {
      $this->loadAgentStatusData($report_data);
      if (isset($report_data['raw']['status']['error'])) {
        $report_data['status'] = FALSE;
      }
      else {
        $report_data['status'] = $this->extractOverviewReportData($report_data['raw']['status']['data'][$report_data['machine_name']]);
      }
    }
    // Confidence report data.
    if (!isset($report_data['confidence'])) {
      $this->loadConfidenceData($report_data);
      if (isset($report_data['raw']['confidence']['error'])) {
        $report_data['confidence'] = FALSE;
      }
      else {
        $report_data['confidence'] = $this->extractConfidenceReportData($report_data['raw']['confidence']['data']['items']);
      }
    }

    // Targeting report data.
    if (!isset($report_data['targeting'])) {
      $this->loadTargetingData($report_data);
      if (isset($report_data['raw']['targeting']['error']) || !isset($report_data['raw']['targeting']['data']['items'])) {
        $report_data['targeting'] = FALSE;
      }
      else {
        $report_data['targeting'] = $this->extractTargetingReportData($report_data['raw']['targeting']['data']['items']);
      }
    }
    if (!isset($report_data['extracted_total'])) {
      // Combine data to form all campaign report data.
      $this->extractCampaignReportData($report_data);
    }
    return $report_data;
  }

  /**
   * Alter the report data returned from API calls to combine into data that is
   * ready for presentation within individual campaign reports.
   *
   * @param array $report_data
   *   The report data loaded and formatted from Acquia Lift.
   *
   * @see loadReportData()
   * @see extractOverviewReportData()
   * @see extractConfidenceReportData()
   * @see extractPotentialTargetingValues()
   * @see extractTargetingReportData()
   */
  protected function extractCampaignReportData(&$report_data) {
    $report_data['extracted_total'] = TRUE;

    if (!is_array($report_data['status']) || !is_array($report_data['confidence']) || !is_array($report_data['targeting']) || !is_array($report_data['potential_context'])) {
      return;
    }
    $agent_data = $this->agent->getData();
    $isAdaptive = $agent_data['decision_style'] === 'adaptive';

    // Determine overall confidence based on confidence in choices.
    $total_confident = 0;
    if (isset($report_data['confidence']['features'][self::NO_FEATURES])) {
      foreach ($report_data['confidence']['features'][self::NO_FEATURES] as $choice) {
        if ($choice['significant']) {
          $total_confident++;
        }
      }
    }

    // Determine the overall time running for this agent.
    $interval_start = new DateTime();
    $interval_start->setTimestamp($this->agent->getStartTime());
    $interval = date_diff($interval_start, date_create());

    // Update the status report data (used for overview report).
    foreach($report_data['status'] as &$report) {
      $report['total_lift_positive'] = $report['unformatted']['total_lift'] > self::LIFT_THRESHHOLD && $total_confident > 0;
      $report['total_confident'] = $total_confident;
      $report['confidence_level'] = $total_confident > 0 ? 'high' : 'low';
      $report['time_running'] = isset($interval) ? $interval->format('%mm, %dd') : '1d';
    }

    // Update context report data.
    $option_numbers = array();

    if (isset($report_data['confidence']['features'])) {
      foreach ($report_data['confidence']['features'] as $feature_string => $feature) {
        // Get the user-friendly feature label from the possible contextual values.
        $feature_label = $feature_string;
        if (isset($report_data['potential_context'][$feature_string])) {
          $feature_label = $report_data['potential_context'][$feature_string]['label'];
        }

        // This report only shows features that can be targeted.
        if (!isset($report_data['targeting'][$feature_string])) {
          continue;
        }

        // Get the data from the targeting report for this feature.
        $targeting_data = $report_data['targeting'][$feature_string];

        // Create a hash of choice numbers for stability report.
        // Don't show system-defined features.
        if ($targeting_data['system'] === TRUE) {
          continue;
        }
        foreach ($feature as $choice_id => $choice) {
          $report_data['context']['features'][$feature_string][$choice_id] = array(
            'counter' => $choice['counter'],
            'choice_id' => $choice['choice_id'],
            'best' => ($isAdaptive && $targeting_data['favored_selection'] === $choice['raw_label']),
            'decisions' => $choice['decisions'],
            'lift_default' => $choice['control'] ? self::DATA_NA : $choice['lift_default'],
            'lift_default_positive' => $choice['unformatted']['lift_default'] > self::LIFT_THRESHHOLD,
            'lift_random' => $choice['lift_random'],
            'lift_random_positive' => $choice['unformatted']['lift_random'] > self::LIFT_THRESHHOLD,
            'control' => $choice['control'],
            'feature_label' => $feature_label,
            'goals' => $choice['goals'],
            'conversion' => $choice['conversion'],
          );
          $option_numbers[$feature_string . '|' . $choice['raw_label']] = $choice['counter'];
        }
      }
    }

    // Build the experiment report data
    $report_data['experiment']['choices'] = isset($report_data['confidence']['features'][self::NO_FEATURES]) ? $report_data['confidence']['features'][self::NO_FEATURES] : array();

    // Build the stability report data
    foreach ($report_data['targeting'] as $feature_string => &$feature) {
      if ($feature['system']) {
        unset($report_data['targeting'][$feature_string]);
        continue;
      }
      // Get the user-friendly feature label from the possible contextual values.
      $feature['feature_label'] = $feature['label'];
      if (isset($report_data['potential_context'][$feature_string])) {
        $feature['feature_label'] = $report_data['potential_context'][$feature_string]['label'];
      }
      if (isset($option_numbers[$feature_string . '|' . $feature['favored_selection']])) {
        $feature['favored_selection_number'] = $option_numbers[$feature_string . '|' . $feature['favored_selection']];
      }
      if (!$isAdaptive) {
        unset($feature['favored_selection_number']);
        unset($feature['favored_selection']);
      }
    }
  }

  /**
   * Returns a render array representing the overview report for the given dates.
   *
   * @param array $report_data
   *   All of the reporting data for the campaign.
   * @return array
   *   A render array representing the overview report.
   */
  protected function buildOverviewReport($report_data) {
    $report = $report_data['status'];
    if ($report === FALSE) {
      return array();
    }

    if ($report_data['today_only']) {
      $overview_report = $report_data['status']['today'];
    }
    else {
      $overview_report = $report_data['status']['all'];
    }

    // Create report renderable.
    $build = array();
    $build['test_type'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#title' => $overview_report['test_type'],
      '#description' => t('test type'),
      '#attributes' => array(
        'id' => 'acquia-lift-overview-type',
      )
    );
    if (isset($overview_report['time_running'])) {
      $build['total_running'] = array(
        '#type' => 'container',
        '#theme' => 'acquia_lift_report_overview',
        '#attributes' => array(
          'id' => 'acquia-lift-overview-running',
        ),
        '#title' => $overview_report['time_running'],
        '#description' => t('total time running'),
      );
    }
    $build['shown'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-shown',
      ),
      '#title' => $overview_report['total_shown'],
      '#description' => format_plural($overview_report['total_shown'], 'time shown', 'times shown'),
    );
    $build['goals'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-goals',
      ),
      '#title' => $overview_report['total_goals'],
      '#description' => t('goals met'),
    );
    if ($overview_report['total_goals_positive']) {
      $build['goals']['#attributes']['class'] = array('acquia-lift-report-positive');
    }
    /*
     * @todo: Figure out a way to present and explain this information so
     * that we can include these figures.
    $build['lift'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-lift',
      ),
      '#title' => $overview_report['total_lift'],
      '#description' => t('Predicted lift/control'),
      '#attributes' => array(
        'class' => array(
          $overview_report['total_lift_positive'] ? 'acquia-lift-report-positive' : 'acquia-lift-report-negative',
        ),
      ),
    );
    $build['confidence'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-confidence',
      ),
      '#title' => t('Confidence'),
      '#description' =>  $overview_report['confidence_level'] == 'high' ? format_plural($overview_report['total_confident'], 'High confidence, 1 var.', 'High confidence, @count vars.') : t('Low confidence'),
      '#attributes' => array(
        'class' => array(
          $overview_report['confidence_level'] == 'high' ? 'acquia-lift-report-positive' : 'acquia-lift-report-negative',
        ),
      ),
      '#total_confident' => $overview_report['total_confident'],
    );
    */
    return $build;
  }

  /**
   * Returns a render array representing the context report.
   *
   * @param array $report_data
   *   Reporting data for the selected dates and decision.
   * @return array
   *   A render array representing the variation set report.
   */
  protected function buildContextReport($report_data) {
    $build = array();

    if ($report_data['confidence'] === FALSE || !isset($report_data['context']['features'])) {
      return array();
    }
    $header = array(
      t('Var.'),
      t('Name'),
      t('Context'),
      t('Shown'),
      t('Goals'),
      t('Conversion rate'),
      t('Lift over control'),
      t('Lift over random'),
    );
    $rows = array();
    foreach($report_data['context']['features'] as $feature_string => $feature) {
      foreach($feature as $choice) {
        $lift_default_classes = array();
        if (!$choice['control']) {
          $lift_default_classes = $choice['lift_default_positive'] ? 'acquia-lift-report-positive' : 'acquia-lift-report-negative';
        }
        $row = array();
        $row[] = $this->getVariationLabel(($choice['counter']-1), $choice['control']);
        if ($choice['control']) {
          $row[] = t('Control: ') . $choice['choice_id'];
        }
        else {
          $row[] = $choice['choice_id'];
        }
        $row[] = array(
          'data' => $choice['best'] ?  $choice['feature_label'] . ' <span class="acquia-lift-best">' . t('best') . '</span>' : $choice['feature_label'],
          'class' => $choice['best'] ? array('acquia-lift-context-best') : array(),
        );
        $row[] = $choice['decisions'];
        $row[] = $choice['goals'];
        $row[] = $choice['conversion'];
        $row[] = array(
          'data' => $choice['lift_default'],
          'class' => $lift_default_classes,
        );
        $row[] = array(
          'data' => $choice['lift_random'],
          'class' => $choice['lift_random_positive'] ? array('acquia-lift-report-positive') : array('acquia-lift-report-negative'),
        );
        $rows[] = array(
          'data' => $row,
          'class' => $choice['control'] ? array('acquia-lift-report-control') : array(),
          'no_striping' => $choice['control'],
          'data-acquia-lift-feature' => $feature_string,
        );
      }
    }

    $build['content'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
      '#sticky' => FALSE,
    );
    return $build;
  }

  /**
   * Returns a render array representing the stability report for targeting
   * features for the given dates.
   *
   * @param array $report_data
   *   Reporting data for the selected dates and decision.
   * @return array
   *   A render array representing the targeting report
   */
  protected function buildStabilityReport($report_data) {
    $build = array();
    if ($report_data['targeting'] === FALSE) {
      return array();
    }

    $data = $report_data['targeting'];

    $rows = array();
    $show_favored_selection = FALSE;
    foreach ($data as $feature => $f) {
      $row_data = array($f['feature_label']);
      if (isset($f['favored_selection_number'])) {
        $show_favored_selection = TRUE;
        $row_data[] = $this->getVariationLabel(($f['favored_selection_number']-1), ($f['favored_selection_number'] == 1));
      }
      $row_data[] = array(
        'data' => $f['percent_traffic'],
        'class' => array($f['percent_traffic_graph']),
      );
      $row_data[] = array(
        'data' => $f['stability'],
        'class' => $f['stability_positive'] ? array('acquia-lift-report-positive') : array('acquia-lift-report-negative'),
      );
      $rows[] = array(
        'data' => $row_data,
        'data-acquia-lift-feature' => $feature,
      );
    }
    $header[] = t('Context');
    if ($show_favored_selection) {
      $header[] = t('Best Variation');
    }
    $header[] = t('Percent of Traffic');
    $header[] = t('Stability');
    $build['content'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
      '#sticky' => FALSE,
    );
    return $build;
  }

  /**
   * Returns a form input array for the context selector.
   *
   * @param array $report_data
   *   Reporting data for the selected dates and decision.
   * @return array
   *   A render array representing the variation set report.
   */
  protected function buildReportContextSelection($report_data) {
    if ($report_data['potential_context'] === FALSE || $report_data['targeting'] === FALSE) {
      return array();
    }
    $context_values = array();
    foreach($report_data['targeting'] as $code => $feature) {
      if ($feature['system'] === TRUE) {
        continue;
      }
      if (isset($report_data['potential_context'][$code])) {
        $type = empty($report_data['potential_context'][$code]['type']) ? t('Other') : $report_data['potential_context'][$code]['type'];
        $context_values[$type][$code] = $report_data['potential_context'][$code]['name'];
      }
      else {
        $type = t('Other');
        $context_values[$type][$code] = $code;
      }
    }
    if (count($context_values) <= 1) {
      return array();
    }
    return array(
      '#title' => t('Context: '),
      '#type' => 'select',
      '#options' => $context_values,
      '#multiple' => TRUE,
    );
  }

  /**
   * Extracts the required confidence report data from the items returned by Acquia Lift.
   *
   * @param $items
   *   An array of items as return from Acquia Lift.
   * @return array
   *   An associative array with information about the performance of each choice.
   */
  protected function extractConfidenceReportData($items) {
    if (empty($items)) {
      return array();
    }
    $data = array(
      'point' => $items[0]['point'],
      'features' => array(),
      'goal_value_differential' => FALSE,
    );
    $last_group = '';
    $counter = 1;
    foreach ($items as $i => $item) {
      // Check to see if we are in a new grouping of choices.
      $check = $item['feature'] . '|' . $item['point'];
      if ($last_group !== $check) {
        $last_group = $check;
        $counter = 1;
      }
      else {
        $counter++;
      }
      $choice = $option_id = $item['choice'];
      $choice_id = $choice;
      if (strpos($choice, ':') !== FALSE) {
        list($decision_name, $option_id) = explode(':', $choice);
        // @todo: Would like to get rid of this function call in order to make
        // the class unit-testable.
        if ($option_label = personalize_get_option_label_for_decision_and_choice($decision_name, $option_id)) {
          $choice_id = $option_label;
        }
      }
      $data['features'][$item['feature']][$choice] = array(
        'unformatted' => array(
          'lift_default' => $item['lift']['default'],
          'lift_random' => $item['lift']['random'],
        ),
        'counter' => $counter,
        'choice_id' => $choice_id,
        'raw_label' => $option_id,
        'decisions' => format_plural($item['totals']['count'], '1 time', '@count times'),
        'goals' => $item['totals']['goals'],
        'value' => $item['totals']['val'],
        'estimated_value' => $this->formatReportNumber($item['vMean'], TRUE, 4),
        'estimated_lower' => $this->formatReportNumber($item['bLo'], TRUE, 4),
        'estimated_higher' => $this->formatReportNumber($item['bHi'], TRUE, 4),
        'goals_per_decision' => $item['totals']['goals'] == 0 ? self::DATA_NA : $this->formatReportNumber($item['totals']['goalsPerDecision'], FALSE),
        'value_per_decision' => $item['totals']['goals'] == 0 ? self::DATA_NA : $this->formatReportNumber($item['totals']['valPerDecision'], FALSE),
        'selections' => $item['count'],
        'conversion' => $item['totals']['goals'] > 0 ? $this->formatReportPercentage($item['totals']['goals'] / $item['totals']['count']) : self::DATA_NA,
        'confidence' => $counter === 1 ? self::DATA_NA : $this->formatReportPercentage($item['confidence']/100),
        'lift_default' => $counter === 1 ? self::DATA_NA : $this->formatReportPercentage($item['lift']['default']/100, TRUE),
        'lift_random' => $this->formatReportPercentage($item['lift']['random']/100, TRUE),
        'significant' => $item['signif'],
        'control' => $counter === 1,
      );
      if (!$data['goal_value_differential'] && ($item['totals']['goals'] != $item['totals']['val'])) {
        $data['goal_value_differential'] = TRUE;
      }
    }
    return $data;
  }

  /**
   * Extracts the required targeting report data from the items returned by Acquia Lift.
   *
   * @param $items
   *   An array of items as return from Acquia Lift.
   * @return array
   *   An associative array with feature codes as keys and associative arrays of info as
   *   values.
   */
  protected function extractTargetingReportData($items) {
    if (empty($items)) {
      return array();
    }
    $data = array();
    foreach ($items as $item) {
      $feature = $item['feature'];
      $favored_selection = 0;
      foreach ($item['choices'] as $i => $choice) {
        // Note: Choices are not in order of option.
        if ($choice['score'] > $item['choices'][$favored_selection]['score']) {
          $favored_selection = $i;
        }
      }
      $data[$feature] = array(
        'raw_label' => $item['label'],
        'label' => $item['labelText'],
        'favored_selection' =>  $item['choices'][$favored_selection]['label'],
        'percent_traffic' => $this->formatReportPercentage($item['percentTraffic']),
        'percent_traffic_graph' => $this->getGraphLevelClass($item['percentTraffic']),
        'predicted_value' => $item['averageResponseValue'],
        'stability' => $this->formatReportNumber($item['stability']),
        'stability_positive' => $item['stability'] > self::STABILITY_THRESHOLD,
        'stability_level' => $item['stabilityLevel'],
        'system' => strpos($item['label'], '[share-alt]') !== FALSE,
      );
    }
    return $data;
  }

  /**
   * Extracts potential targeting values from the raw data returned by Acquia
   * Lift.
   *
   * @param $items
   *   An array of raw potential values.
   * $return array
   *   An associative array of potential targeting features keyed by code.
   */
  protected function extractPotentialTargetingValues($items) {
    $data = array();
    if (isset($items['potential']['features']) && !empty($items['potential']['features'])) {
      foreach ($items['potential']['features'] as $feature) {
        $data[$feature['code']] = array(
          'type' => isset($feature['typeName']) ? $feature['typeName'] : '',
          'name' => ($feature['name'] === '-' || $feature['name'] === '0') ? $feature['code'] : $feature['name'],
        );
        $data[$feature['code']]['label'] = empty($data[$feature['code']]['type']) ? $data[$feature['code']]['name'] : $data[$feature['code']]['type'] . ': ' . $data[$feature['code']]['name'];
      }
    }
    return $data;
  }

  /**
   * Gets the appropriate class name for a graph indicating percentage.
   *
   * @param $value
   *   The percentage value expressed as a number between 0 and 1.
   */
  protected function getGraphLevelClass($value) {
    if ($value >= 1) {
      return 'acquia-lift-graph-level-5';
    }
    else if ($value >= .8) {
      return 'acquia-lift-graph-level-4';
    }
    else if ($value >= .6 && $value < .8) {
      return 'acquia-lift-graph-level-3';
    }
    else if ($value >= .4 && $value < .6) {
      return 'acquia-lift-graph-level-2';
    }
    else if ($value >= .2 && $value < .4) {
      return 'acquia-lift-graph-level-1';
    }
    else {
      return 'acquia-lift-graph-level-0';
    }
  }


  /**
   * Helper to retrieve context filters for legacy reports since this is no longer
   * provided by the API.
   */
  public function getContextFilters($agent_name) {
    if ($this->reportDataSrc instanceof AcquiaLiftReportDataFromFile) {
      $context_filters = $this->reportDataSrc->getReport($agent_name, 'context-filters');
      if (!empty($context_filters)) {
        return $context_filters;
      }
    }
    elseif ($this->reportDataSrc instanceof AcquiaLiftAPI) {
      $path = "/-/potential-targeting?agent={$agent_name}&include-current=true";
      return $this->reportDataSrc->makeLegacyGetRequest($path, array(), 'Problem retrieving potential targeting values');
    }
    return array(
      'data' => array(
        'potential' => array(
          'features' => array()
        )
      )
    );
  }
}

function acquia_lift_legacy_report_form(&$form, &$form_state, $reports) {
  // The context and stability reports are only relevant if there is context
  // targeting in place for this campaign.
  if (empty($reports['targeting'])) {
    return;
  }
  module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');
  // Context report section.
  $context_select = $reports['targeting'];
  acquia_lift_chosenify_element($context_select, array('acquia-lift-chosen-select-half', 'acquia-lift-report-context-select'));
  $form['context_report'] = array(
    '#type' => 'container',
    'header' => array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('acquia-lift-report-section-header', 'clearfix'),
      ),
      'title' => array(
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('acquia-lift-report-section-title'),
        ),
        'report_title' => array(
          '#markup' => '<h2>' . t('Context') . '</h2>',
        ),
      ),
      'summary' => array(
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('acquia-lift-report-header-summary'),
        ),
      ),
    ),
    '#attributes' => array(
      'id' => 'acquia-lift-context-report',
      'class' => array('acquia-lift-report-section'),
    )
  );
  $form['context_report']['header']['summary']['report_summary'] = array(
    '#theme_wrappers' => array('container'),
    '#markup' => t('See who converts best for each content variation'),
    '#attributes' => array(
      'class' => array('acquia-lift-report-summary'),
    ),
  );
  $form['context_report']['header']['controls'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-report-controls'),
    ),
    'context' => $context_select,
  );
  $form['context_report']['report'] = array(
    '#markup' => drupal_render($reports['context']),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-context-report-data',
  );

  // Stability report section.
  $context_select = $reports['targeting'];
  acquia_lift_chosenify_element($context_select, array('acquia-lift-chosen-select-half', 'acquia-lift-report-context-select'));
  $form['advanced_reports'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced reporting'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['advanced_reports']['stability_report'] = array(
    '#type' => 'container',
    'header' => array(
      '#type' => 'container',
      '#attributes' => array('acquia-lift-report-section-header', 'clearfix'),
      'title' => array(
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('acquia-lift-report-section-title'),
        ),
        'report_title' => array(
          '#markup' => '<h2>' . t('Context Stability') . '</h2>',
        ),
      ),
      'summary' => array(
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('acquia-lift-report-header-summary'),
        ),
      ),
    ),
    '#attributes' => array(
      'id' => 'acquia-lift-stability-report',
      'class' => array('acquia-lift-report-section'),
    ),
  );
  if (!empty($report_title_additional)) {
    $form['advanced_reports']['stability_report']['header']['title']['groups'] = array(
      '#markup' => t('random and personalized groups'),
    );
    $form['advanced_reports']['stability_report']['header']['summary']['distribution'] = array(
      '#markup' => $report_title_additional,
      '#theme_wrappers' => array('container'),
    );
  }
  $form['advanced_reports']['stability_report']['header']['controls'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-report-controls'),
    ),
    'context' => $context_select,
  );
  $form['advanced_reports']['stability_report']['report'] = array(
    '#markup' => drupal_render($reports['stability']),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-stability-report-data',
  );

  // We have to specify the include file so as not to lose it during rendering from ajax.
  // @see drupal_retrieve_form():734
  $form_state['build_info']['files'] = array(
    drupal_get_path('module', 'acquia_lift') . '/acquia_lift.admin.inc',
    drupal_get_path('module', 'acquia_lift') . '/acquia_lift.ui.inc',
  );
}
