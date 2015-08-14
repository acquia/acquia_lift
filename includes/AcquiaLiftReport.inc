<?php

/**
 * Class for Acquia Lift Reports.
 */
class AcquiaLiftReport extends AcquiaLiftReportBase {

  /**
   * The value to show when report data is not applicable.
   */
  const DATA_NA = '&mdash;';

  /**
   * The threshold to use above which lift percentage will be positively noted.
   */
  const LIFT_THRESHHOLD = 0;

  /**
   * Holds the data used for per-day reports.
   *
   * @var array
   */
  protected $dailyReportData;

  /**
   * Holds the data used for the status report.
   *
   * @var array
   */
  protected $statusReportData;

  /**
   * Holds the data used for aggregated reports.
   *
   * @var array
   */
  protected $aggregatedData;

  /**
   * Constructs an AcquiaLiftReport object.
   *
   * @param \PersonalizeAgentInterface $agent
   *   THe agent the report is for.
   * @param array $options
   *   An array of configuration options for the report.
   * @param \AcquiaLiftReportDataSourceInterface $report_data_src
   *   A AcquiaLiftReportDataSourceInterface object to retrieve the actual data
   *   from.
   */
  function __construct(PersonalizeAgentInterface $agent, $options, AcquiaLiftReportDataSourceInterface $report_data_src) {
    parent::__construct($agent, $options);
    $this->reportDataSrc = $report_data_src;
  }

  /**
   * Gets the render array for the main reporting page.
   *
   * @return array
   *   An array of output to pass to drupal_render.
   */
  public function getCampaignReports() {
    return $this->buildCampaignReports(
      $this->getSessionCount(),
      $this->getGoalCount(),
      $this->getDailyData(),
      $this->getAggregatedData(),
      $this->getOverallConfidence(),
      $this->getOverallWinner());
  }

  /**
   * Returns a render array for the experimental portion of the reports.
   *
   * @return array
   */
  public function getExperimentReports() {
    $config = $this->getReportConfiguration();
    $report_name = t('All goals');
    $goal = NULL;
    if (!empty($config['goal'])) {
      $goal = $config['goal'];
      $actions = visitor_actions_get_actions();
      // If the action still exists, use the label, otherwise just use the goal
      // machine name.
      if (isset($actions[$goal])) {
        $report_name = $actions[$goal]['label'];
      }
      else {
        $report_name = $goal;
      }
    }

    return $this->buildExperimentReports(
      $report_name,
      $this->getDailyData(),
      $this->getAggregatedData(),
      $this->getOverallConfidence(),
      $this->getOverallWinner());
  }


  /**
   * Gets the agent status report from the report data source and stores it.
   *
   * @return array
   *   An array representing the status report for the agent.
   */
  protected function getStatusReportData() {
    $config = $this->getReportConfiguration();
    if (empty($this->statusReportData)) {
      try {
        $interval = date_diff(date_create($config['date_from']), date_create($config['date_to']));
        $num_days = $interval->days;
        $agent_name = $config['machine_name'];
        $report_data = $this->reportDataSrc->getAgentStatusReport(array($agent_name), $num_days);
        if (empty($report_data['data'])) {
          // If there's no data we should not try to retrieve it again so
          // populate the statusReportData variable with an error.
          $this->hasData = FALSE;
          return array();
        }
        $this->statusReportData = reset($report_data['data']);
      }
      catch (Exception $e) {
        $this->hasData = FALSE;
        $this->statusReportData = array(
          'error' => $e->getMessage()
        );
        return array();
      }
    }
    return $this->statusReportData;
  }

  /**
   * Returns the total number of sessions for this campaign.
   *
   * @return int
   */
  protected  function getSessionCount() {
    $status_report_data = $this->getStatusReportData();
    if (!isset($status_report_data['totals']['sessions']['count'])) {
      $this->hasData = FALSE;
      return 0;
    }
    return $status_report_data['totals']['sessions']['count'];
  }

  /**
   * Returns the total number of goals for this campaign.
   *
   * @return int
   */
  protected function getGoalCount() {
    $status_report_data = $this->getStatusReportData();
    if (!isset($status_report_data['totals']['goals']['count'])) {
      $this->hasData = FALSE;
      return 0;
    }
    return $status_report_data['totals']['goals']['count'];
  }

  /**
   * Gets the per-day confidence report from the report data source and stores it.
   *
   * @return array
   *   An array of per-day reports.
   */
  protected function getDailyData() {
    $config = $this->getReportConfiguration();
    if (empty($this->dailyReportData)) {
      try {
        $options = array(
          'confidence-measure' => $config['confidence_measure'],
          'aggregated-over-dates' => FALSE,
        );
        if (!empty($config['goal'])) {
          $options['goal'] = $config['goal'];
        }
        $report_data = $this->reportDataSrc->getConfidenceReport(
          $config['machine_name'],
          $config['date_from'],
          $config['date_to'],
          $config['decision_name'],
          $options
        );
        if (empty($report_data) || !isset($report_data['data']) || empty($report_data['data']['items'])) {
          $this->hasData = FALSE;
          // If there's no data we should not try to retrieve it again so
          // populate the dailyReportData variable with an error.
          $this->dailyReportData = array(
            'error' => t('No data')
          );
          return array();
        }
      }
      catch (Exception $e) {
        $this->hasData = FALSE;
        $this->dailyReportData = array(
          'error' => $e->getMessage()
        );
        return array();
      }
      $items = $report_data['data']['items'];
      $data = $option_ids = array();
      foreach ($items as $item) {
        if ($item['feature'] != AcquiaLiftAPI::NO_FEATURES) {
          continue;
        }
        $option_id = $option_label = $item['choice'];
        if (strpos($item['choice'], ':') !== FALSE) {
          list($decision_name, $option_id) = explode(':', $item['choice']);
          $option_label = $option_id;
          if ($label = personalize_get_option_label_for_decision_and_choice($decision_name, $option_id)) {
            $option_label = $label;
          }
        }

        // Keep track of the option's index so that we know whether or not
        // it's the control option.
        if (!in_array($option_id, $option_ids)) {
          $option_ids[] = $option_id;
        }
        $idx = array_search($option_id, $option_ids);
        $goals = $item['goals'];
        $count = $item['count'];
        $val = $item['val'];
        $total_count[$idx] = isset($total_count[$idx]) ? $total_count[$idx] + $count : $count;
        $total_goals[$idx] = isset($total_goals[$idx]) ? $total_goals[$idx] + $goals : $goals;
        $total_val[$idx] = isset($total_val[$idx]) ? $total_val[$idx] + $val : $val;
        $rate = $total_count[$idx] > 0 ? ($total_goals[$idx]/$total_count[$idx]) * 100 : 0;
        $val_rate = $total_count[$idx] > 0 ? ($total_val[$idx]/$total_count[$idx]) : 0;
        $margin = ($item['bHi'] - $item['bLo']) / 2;

        $data[] = array(
          'option_id' => $option_id,
          'option_label' => $option_label,
          'goals' => $total_goals[$idx],
          'count' => $total_count[$idx],
          'date' => $item['date'],
          'timestamp' => strtotime($item['date']),
          'conversion' => $this->formatReportNumber($rate, TRUE, 4),
          'conversion_value' => $this->formatReportNumber($val_rate, TRUE, 4),
          'estimated_value' => $this->formatReportNumber($item['vMean'], TRUE, 4),
          'margin_error' => $this->formatReportNumber($margin, TRUE, 4),
          'counter' => $idx,
          'control' => $idx === 0,
        );
      }
      $this->dailyReportData = $data;
    }
    return $this->dailyReportData;
  }

  /**
   * Retrieves the aggregated confidence report data from the report data source.
   *
   * @return array
   *   An associative array with the following keys
   *   - data: the per-variation reports
   *   - overview: an associative array with
   *     - confidence: Boolean indicating whether the report has confidence.
   *     - winner: The index of the winning variation, if there is one.
   */
  protected function getAggregatedConfidenceReport() {
    $config = $this->getReportConfiguration();
    if (empty($this->aggregatedData)) {
      try {
        // Pass the correct options to our data source.
        $options = array(
          'confidence-measure' => $config['confidence_measure'],
          'aggregated-over-dates' => TRUE,
        );
        if (!empty($config['goal'])) {
          $options['goal'] = $config['goal'];
        }
        $report_data = $this->reportDataSrc->getConfidenceReport(
          $config['machine_name'],
          $config['date_from'],
          $config['date_to'],
          $config['decision_name'],
          $options
        );
        if (empty($report_data) || !isset($report_data['data']) || empty($report_data['data']['items'])) {
          $this->hasData = FALSE;
          // If there's no data we should not try to retrieve it again so
          // populate the aggregatedData variable with an error.
          $this->aggregatedData = array(
            'error' => t('No data')
          );
          return array();
        }
      }
      catch (Exception $e) {
        $this->hasData = FALSE;
        $this->aggregatedData = array(
          'error' => $e->getMessage()
        );
        return array();
      }

      // Go through the items and extract the information we need.
      $items = $report_data['data']['items'];
      $data = array();
      $confidence = FALSE;
      $winner = '';
      $winning_value = NAN;
      foreach ($items as $i => $item) {
        if ($item['feature'] !== AcquiaLiftAPI::NO_FEATURES) {
          continue;
        }
        $option_id = $option_label = $item['choice'];
        if (strpos($item['choice'], ':') !== FALSE) {
          list($decision_name, $option_id) = explode(':', $item['choice']);
          $option_label = $option_id;
          if ($label = personalize_get_option_label_for_decision_and_choice($decision_name, $option_id)) {
            $option_label = $label;
          }
        }

        $goals = $item['goals'];
        $count = $item['count'];
        // The format for percentages will already multiply by 100.
        $rate = $count > 0 ? ($goals/$count) : 0;
        $margin = ($item['bHi'] - $item['bLo']) / 2;
        if ($item['signif'] && $item['confidence'] > $config['confidence_measure']) {
          if (is_nan($winning_value) || $item['confidence'] > $winning_value) {
            $winning_value = $item['confidence'];
            $winner = $i;
            $confidence = TRUE;
          }
        }
        $data[$option_id] = array(
          'counter' => $i,
          'option_id' => $option_id,
          'option_label' => $option_label,
          'goals' => $goals,
          'count' => $count,
          'conversion' => $this->formatReportPercentage($rate),
          'estimated_value' => $this->formatReportNumber($item['vMean'], TRUE, 4),
          'estimated_higher' => $this->formatReportNumber($item['bHi'], TRUE, 4),
          'estimated_lower' => $this->formatReportNumber($item['bLo'], TRUE, 4),
          'margin_error' => $this->formatReportNumber($margin, TRUE, 4),
          'significant' => $item['signif'],
          'control' => $i === 0,
          'confidence' => $i === 0 ? self::DATA_NA : $this->formatReportPercentage($item['confidence']/100),
          'lift_default' => $i === 0 ? self::DATA_NA : $this->formatReportPercentage($item['lift']['default']/100, TRUE),
          'lift_random' => $this->formatReportPercentage($item['lift']['random']/100, TRUE),
        );
      }
      $this->aggregatedData = array(
        'data' => $data,
        'overview' => array(
          'confidence' => $confidence,
          'winner' => $winner,
        ),
      );
    }
    return $this->aggregatedData;
  }

  /**
   * Returns the per-variation confidence report data aggregated over dates.
   *
   * @return array
   *   An array of report data keyed by option id.
   */
  protected function getAggregatedData() {
    $data = $this->getAggregatedConfidenceReport();
    return isset($data['data']) ? $data['data'] : array();
  }

  /**
   * Gets the overall confidence of the report, based on aggregated data.
   *
   * @return bool
   *   TRUE if the report has overall confidence, FALSE otherwise.
   */
  protected function getOverallConfidence() {
    $data = $this->getAggregatedConfidenceReport();
    return isset($data['overview']['confidence']) ? $data['overview']['confidence'] : FALSE;
  }

  /**
   * Gets the overall winner of the campaign, based on aggregated data.
   *
   * @return int|NULL
   *   An integer representing the index of the winner variation, or NULL if
   *   there isn't one.
   */
  protected function getOverallWinner() {
    $data = $this->getAggregatedConfidenceReport();
    return isset($data['overview']['winner']) ? $data['overview']['winner'] : NULL;
  }

}
