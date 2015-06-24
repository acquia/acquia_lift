<?php
/**
 * @file
 * Provides an agent type for Acquia Lift
 */


class AcquiaLiftAgent extends PersonalizeAgentBase implements PersonalizeAgentGoalInterface, AcquiaLiftLearningAgentInterface {

  /**
   * An object containing the agent data.
   *
   * @var stdClass
   */
  protected $agent;

  /**
   * An instance of AcquiaLiftAPI.
   *
   * @var AcquiaLiftAPI
   */
  protected $liftAPI;

  /**
   * A report object.
   *
   * The agent acts as a facade to the reporting class for reporting requests.
   */
  protected $reporting;

  protected $globalConfig;

  /**
   * Implements PersonalizeAgentInterface::create().
   */
  public static function create($agent_data) {
    try {
      $acquia_lift_api = AcquiaLiftAPI::getInstance(variable_get('acquia_lift_account_info', array()));
      $status = personalize_agent_get_status($agent_data->machine_name);
      $config = array(
        'confidence_measure' => variable_get('acquia_lift_confidence_measure', 95),
        'minimum_runtime' => acquia_lift_config_min_runtime(),
        'minimum_decisions' => variable_get('acquia_lift_min_decisions', 1000)
      );
      return new static($agent_data->machine_name, $agent_data->label, $agent_data->data, $status, !empty($agent_data->started) ? $agent_data->started : NULL, $acquia_lift_api, $config);
    }
    catch (AcquiaLiftException $e) {
      watchdog('Acquia Lift', 'Unable to instantiate Acquia Lift Agent');
      return NULL;
    }
  }

  /**
   * Constructs an Acquia Lift agent.
   *
   * @param stdClass $agent_data
   *   An object containing the agent data.
   * @param $acquia_lift_api
   *   An instance of the AcquiaLiftAPI class.
   */
  public function __construct($machine_name, $title, $data, $status, $started, AcquiaLiftAPI $acquia_lift_api, $global_config) {
    parent::__construct($machine_name, $title, $data, $status, $started);
    $this->liftAPI = $acquia_lift_api;
    $this->globalConfig = $global_config;
  }

  /**
   * Implements PersonalizeAgentInterface::getType();
   */
  public function getType() {
    return 'acquia_lift';
  }

  /**
   * Implements PersonalizeAgentInterface::getAssets().
   */
  public function getAssets() {
    $path = drupal_get_path('module', 'acquia_lift');
    return array(
      'js' => array(
        array(
          'type' => 'setting',
          'data' => array(
            'acquia_lift' => array(
              'apiKey' => $this->liftAPI->getApiKey(),
              'owner' => $this->liftAPI->getOwnerCode(),
              'baseUrl' => $this->liftAPI->getApiUrl(),
              'stringReplacePattern' => AcquiaLiftAPI::STRING_REPLACE_PATTERN,
              'batchMode' => variable_get('acquia_lift_batch_decisions', FALSE)
            )
          )
        ),
        // Add the Acquia Lift API js wrapper and the Acquia Lift integration js.
        $path . '/js/acquia_lift.js' => array('type' => 'file', 'scope' => 'footer', 'defer' => TRUE),
      ),
      'library' => array(
        array('acquia_lift', 'acquia_lift.agent_api'),
      ),
    );
  }

  /**
   * Implements PersonalizeAgentGoalInterface::useClientSideGoalDelivery().
   */
  public function useClientSideGoalDelivery() {
    return variable_get('acquia_lift_client_side_goals', TRUE);
  }

  /**
   * Implements PersonalizeAgentGoalInterface::sendGoal().
   */
  public function sendGoal($goal_name, $value = NULL) {
    // @todo Implement server-side goal delivery.
  }

  /**
   * Returns the operations needed to sync an agent to Acquia Lift.
   *
   * @param $targeting_rule_exists
   *   Whether the existing agent in Lift has a targeting rule set up that may
   *   need to be deleted.
   * @return array
   *   An array of items representing API calls to be made to Acquia Lift.
   */
  public function getAgentSyncOperations() {
    $items = array();
    $acquia_lift_control_rate = .1;
    $acquia_lift_explore_rate = .2;
    if (isset($this->data['control_rate'])) {
      // Acquia Lift takes the control rate as a number between 0 and 1.
      $acquia_lift_control_rate = $this->data['control_rate'] / 100;
    }
    if (isset($this->data['explore_rate']) && isset($this->data['decision_style'])) {
      if ($this->data['decision_style'] === 'adaptive') {
        // Acquia Lift takes the explore rate as a number between 0 and 1.
        $acquia_lift_explore_rate = $this->data['explore_rate'] / 100;
      }
      else {
        // If the decision style is to only test, then the explore rate is the
        // full group.
        $acquia_lift_explore_rate = 1;
      }
    }
    // Add an item for saving the agent to Acquia Lift.
    $items[] = array(
      'method' => 'saveAgent',
      'args' => array(
        $this->machineName,
        $this->title,
        $this->data['decision_style'],
        // Always set the agent to enabled in Lift so that status changes don't
        // need to be sync'd.
        AcquiaLiftAPI::ENABLED_STATUS,
        $acquia_lift_control_rate,
        $acquia_lift_explore_rate,
        isset($this->data['cache_decisions']) && $this->data['cache_decisions']
      ),
    );

    return $items;
  }

  /**
   * Get a reference to this agent's reporting class.
   *
   * Allows for delayed instantiation of reporting class.
   */
  protected function getReporting($options) {
    if (empty($this->reporting)) {
      $this->reporting = AcquiaLiftReportFactory::create($this, $this->liftAPI, $options);
    }
    return $this->reporting;
  }

  /**
   * Implements AcquiaLiftLearningAgentInterface::getCampaignReports().
   *
   * A facade to the reporting class.
   */
  public function getCampaignReports($options) {
    return $this->getReporting($options)->getCampaignReports();
  }

  /**
   * Implements AcquiaLiftLearningAgentInterface()::getExperimentReports().
   */
  public function getExperimentReports($options) {
    return $this->getReporting($options)->getExperimentReports();
  }

  /**
   * Implements AcquiaLiftAgent::convertOptionSetsToDecisions().
   */
  public static function convertOptionSetsToDecisions($option_sets) {
    $points = array();
    foreach ($option_sets as $option_set) {
      // If for some reason one of our option sets is missing a point name or
      // decision name, throw an exception as we cannot proceed.
      if (!isset($option_set->decision_point) || !isset($option_set->decision_name)) {
        throw new AcquiaLiftException('Cannot convert option sets to a structured decision hierarchy without decision points and decision names');
      }
      $points[$option_set->decision_point] = isset($points[$option_set->decision_point]) ?
        $points[$option_set->decision_point] : array();
      $points[$option_set->decision_point][$option_set->decision_name] = isset($points[$option_set->decision_point][$option_set->decision_name]) ?
        $points[$option_set->decision_point][$option_set->decision_name] : array();
      foreach ($option_set->options as $option) {
        $points[$option_set->decision_point][$option_set->decision_name][] = $option['option_id'];
      }
    }
    return $points;
  }

  /**
   * Converts an exception thrown by the API class into errors added to the passed in array.
   *
   * @param AcquiaLiftException $e
   *   The excetion that was thrown.
   * @param $errors
   *   An array of errors to add to.
   * @return array
   *   The new errors array.
   */
  protected function convertAgentExceptionToErrors(AcquiaLiftException $e, &$errors) {
    if ($e instanceof AcquiaLiftNotFoundException) {
      $errors[] = t('This agent has not yet been pushed to Acquia Lift');
    }
    else {
      $errors[] = t('There was a problem communicating with the Acquia Lift server.');
    }
    return $errors;
  }

  /**
   * Returns the operations needed to sync decisions to Acquia Lift.
   *
   * @param $old_decisions
   *   An array representing the old decisions Acquia Lift knows about.
   * @param $new_decisions
   *   An array representing the new decisions.
   * @return array
   *   An array of items representing API calls to be made to Acquia Lift.
   */
  public function getDecisionSyncOperations($old_decisions, $new_decisions) {
    $items = array();
    // Save everything in $new_decisions to Acquia Lift.
    foreach ($new_decisions as $point => $decisions) {
      $items[] = array(
        'method' => 'savePoint',
        'args' => array(
          $this->machineName,
          $point
        )
      );
      foreach ($decisions as $decision_name => $choices) {
        $items[] = array(
          'method' => 'saveDecision',
          'args' => array(
            $this->machineName,
            $point,
            $decision_name
          )
        );
        foreach ($choices as $choice) {
          $items[] = array(
            'method' => 'saveChoice',
            'args' => array(
              $this->machineName,
              $point,
              $decision_name,
              $choice
            )
          );
        }
      }
    }
    // Now remove anything that was in $old_decisions but not in
    // $new_decisions.
    foreach ($old_decisions as $point => $decisions) {
      if (!isset($new_decisions[$point])) {
        $items[] = array(
          'method' => 'deletePoint',
          'args' => array(
            $this->machineName,
            $point
          )
        );
      }
      else {
        foreach ($decisions as $decision_name => $choices) {
          if (!isset($new_decisions[$point][$decision_name])) {
            // Delete this decision from the decision point.
            $items[] = array(
              'method' => 'deleteDecision',
              'args' => array(
                $this->machineName,
                $point,
                $decision_name
              )
            );
          }
          else {
            foreach ($choices as $choice) {
              if (!in_array($choice, $new_decisions[$point][$decision_name])) {
                // Delete this choice from the decision.
                $items[] = array(
                  'method' => 'deleteChoice',
                  'args' => array(
                    $this->machineName,
                    $point,
                    $decision_name,
                    $choice
                  )
                );
              }
            }
          }
        }
      }
    }
    return $items;
  }

  /**
   * Returns the operations needed to sync goals to Acquia Lift.
   *
   * @param $old_goals
   *   An array representing the old goals Acquia Lift knows about.
   * @param $new_goals
   *   An array representing the new goals.
   * @return array
   *   An array of items representing API calls to be made to Acquia Lift.
   */
  public function getGoalSyncOperations($old_goals, $new_goals) {
    $items = array();
    // Save the new goals to Acquia Lift
    foreach ($new_goals as $goal_name => $goal_value) {
      $items[] = array(
        'method' => 'saveGoal',
        'args' => array(
          $this->machineName,
          $goal_name
        )
      );
    }
    // Now delete any old goals that are not in the new goals array.
    foreach ($old_goals as $goal_name => $goal_value) {
      if (!isset($new_goals[$goal_name])) {
        $items[] = array(
          'method' => 'deleteGoal',
          'args' => array(
            $this->machineName,
            $goal_name
          )
        );
      }
    }
    return $items;
  }
}

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
        if (!isset($report_data['data']) || !isset($report_data['data'][$agent_name])) {
          // If there's no data we should not try to retrieve it again so
          // populate the statusReportData variable with an error.
          $this->hasData = FALSE;
          return array();
        }
        $this->statusReportData = $report_data['data'][$agent_name];
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
    return (int) $this->formatReportNumber($status_report_data['totals']['sessions']['count'], TRUE, 0);
  }

  /**
   * Returns the total number of sessions for this campaign.
   *
   * @return int
   */
  protected function getGoalCount() {
    $status_report_data = $this->getStatusReportData();
    if (!isset($status_report_data['totals']['goals']['count'])) {
      $this->hasData = FALSE;
      return 0;
    }
    return (int) $this->formatReportNumber($this->formatReportNumber($status_report_data['totals']['goals']['count']), TRUE, 0);
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

        $goals = $item['totals']['goals'];
        $count = $item['totals']['count'];
        $val = $item['totals']['val'];
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

        $goals = $item['totals']['goals'];
        $count = $item['totals']['count'];
        // The format for percentages will already multiply by 100.
        $rate = $count > 0 ? ($goals/$count) : 0;
        $margin = ($item['bHi'] - $item['bLo']) / 2;
        if ($item['signif']) {
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

