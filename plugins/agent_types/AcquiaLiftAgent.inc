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
   * Implements AcquiaLiftAgentInterface::buildCampaignReports().
   *
   * A facade to the reporting class.
   */
  public function buildCampaignReports($options) {
    return $this->getReporting($options)->getCampaignReports();
  }

  /**
   * Implements AcquiaLiftReportInterface()::buildConversionReport().
   */
  public function buildConversionReport($options) {
    return $this->getReporting($options)->getConversionReport();
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
 * Base class providing report data loading functionality common to all
 * Acquia Lift Reports.
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

  function __construct(PersonalizeAgentInterface $agent, $options, AcquiaLiftReportDataSourceInterface $report_data_src) {
    parent::__construct($agent, $options);
    $this->reportDataSrc = $report_data_src;
  }

  public function getCampaignReports() {
    $this->loadReportData();
    return $this->buildCampaignReports(
      $this->getSessionCount(),
      $this->getGoalCount(),
      $this->getDailyData(),
      $this->getAggregatedData(),
      $this->getOverallConfidence(),
      $this->getOverallWinner());
  }

  public function getConversionReport() {
    $config = $this->getReportConfiguration();
    $report_name = t('All goals');
    $goal = NULL;
    if (!empty($config['goal'])) {
      $goal = $config['goal'];
      // Get the summary data for the whole campaign in order to have overall
      // statistics.
      $this->loadConversionReportHelper('confidence_goal', TRUE, $goal);
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
    $this->loadConversionReportHelper('confidence_detail', FALSE);
    $this->loadConversionReportHelper('confidence', TRUE);

    $reports = $this->buildConversionReports(
      $report_name,
      $this->getDailyData(),
      $this->getAggregatedData(),
      $this->getOverallConfidence(),
      $this->getOverallWinner());
    return $reports;
  }

  /**
   * Loads the agent status raw reporting data.
   */
  protected function loadAgentStatusData() {
    $config = $this->getReportConfiguration();
    // Check and see if it is already loaded.
    if (isset($this->loadedReports['raw']['status'])) {
      return;
    }
    try {
      if ($config['today_only']) {
        $num_days = 1;
      }
      else {
        $interval = date_diff(date_create($config['date_from']), date_create($config['date_to']));
        $num_days = $interval->days;
      }
      $this->loadedReports['raw']['status'] = $this->reportDataSrc->getAgentStatusReport(array($config['machine_name']), $num_days);
    }
    catch (Exception $e) {
      $this->loadedReports['raw']['status']['error'] = $e->getMessage();
    }
  }

  /**
   * Loads the agent confidence raw reporting data.
   *
   * @param $report_name
   *   The report name to use when adding this to the loadedReports property.
   * @param $aggregated
   *   (Optional) Whether to aggregate over dates.
   * @param $goal
   *   (Optional) A goal to restrict the report to.
   */
  protected function loadConfidenceData($report_name, $aggregated = TRUE, $goal = NULL) {
    // Check and see if it is already loaded.
    if (isset($this->loadedReports['raw'][$report_name])) {
      return;
    }

    $config = $this->getReportConfiguration();
    try {
      $options = array(
        'confidence-measure' => $config['confidence_measure'],
        'aggregated-over-dates' => $aggregated,
      );
      if (!empty($goal)) {
        $options['goal'] = $goal;
      }
      $this->loadedReports['raw'][$report_name] = $this->reportDataSrc->getConfidenceReport(
        $config['machine_name'],
        $config['date_from'],
        $config['date_to'],
        $config['decision_name'],
        $options
      );
    }
    catch (Exception $e) {
      $this->loadedReports['raw'][$report_name]['error'] = $e->getMessage();
    }
  }


  /**
   * Handles all of the logic to load and extract a conversion report.
   *
   * @param $report_name
   *   The report name to use when adding this to the loadedReports property.
   * @param $aggregated
   *   (Optional) Whether to aggregate over dates.
   */
  protected function loadConversionReportHelper($report_name, $aggregated) {
    $config = $this->getReportConfiguration();
    // Limit report results to the experimental group.
    $options = array(
      'policies' => 'explore',
      'aggregated-over-dates' => $aggregated,
    );
    $goal = NULL;
    if (!empty($config['goal'])) {
      $goal = $config['goal'];
      $options['goal'] = $goal;
    }

    if (!empty($goal)) {
      // Add the report name for this goal to track which goals are included.
      $this->loadedReports['goal_reports'][] = $report_name;
      // Determine the parent for reports added by this helper.
      if (!isset($this->loadedReports['conversion_goals'])) {
        $this->loadedReports['conversion_goals'][$goal] = FALSE;
      }
      $detail_report = &$this->loadedReports[$goal];
    }
    else {
      // Determine the parent for reports added by this helper.
      if (!isset($this->loadedReports['conversion_all'])) {
        $this->loadedReports['conversion_all'] = FALSE;
      }
      $detail_report = &$this->loadedReports['conversion_all'];
    }
    // Check if the report is already loaded.
    $subreport_name = $aggregated ? 'summary' : 'detail';
    if (isset($detail_report[$subreport_name])) {
      return;
    }
    // Load the raw report data.
    if (!isset($this->loadedReports['raw'][$report_name])) {
      $this->loadConfidenceData($report_name, $aggregated, $goal);
      if (isset($this->loadedReports['raw'][$report_name]['error'])) {
        return;
      }
    }

    // Extract the data into full report data from the raw data.
    $detail_report[$subreport_name] = $aggregated ? $this->extractConversionSummaryData($report_name) : $this->extractConversionReportData($report_name);
  }

  /**
   * Extracts the required overview data from the report data returned by
   * Acquia Lift.

   * @return array
   *   An associative array with information for today's and the total overview.
   */
  protected function extractOverviewReportData() {
    $items = $this->getStatusReportItems();
    $agent_data = $this->agent->getData();
    $decision_style = $agent_data['decision_style'] === 'adaptive' ? t('Auto-personalize') : t('A/B');
    $total_variations = isset($agent_data['decisions']) ? count($agent_data['decisions']) : 0;
    $report = array();
    // Create an array of data for each time option.
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

  protected function getDailyData() {
    return $this->extractConversionReportData();
  }

  protected function getAggregatedData() {
    $data = $this->extractConversionSummaryData();
    return $data['data'];
  }

  protected function getOverallConfidence() {
    $data = $this->extractConversionSummaryData();
    return $data['overview']['confidence'];
  }

  protected function getOverallWinner() {
    $data = $this->extractConversionSummaryData();
    return $data['overview']['winner'];
  }

  protected function getStatusReportItems() {
    $config = $this->getReportConfiguration();
    $status_report = $this->loadedReports['raw']['status'];
    if (!isset($status_report['data']) || !isset($status_report['data'][$config['machine_name']])) {
      return array();
    }
    return $status_report['data'][$config['machine_name']];
  }

  /**
   * Extracts data from the raw confidence detail report that is prepared for use
   * within the conversion report rendering process.
   *
   * @return array
   *   An associative array with information about the performance of each choice.
   */
  protected function extractConversionReportData() {
    $config = $this->getReportConfiguration();
    $report_name = 'confidence_detail';
    if (!empty($config['goal'])) {
      $report_name = 'confidence_goal_detail';
    }
    $report = $this->loadedReports['raw'][$report_name];
    if (!isset($report['data']) || empty($report['data']['items'])) {
      return array();
    }
    $items = $report['data']['items'];

    $shown = array();
    $counter = NAN;
    $data = array();
    foreach ($items as $item) {
      if ($item['feature'] != AcquiaLiftAPI::NO_FEATURES) {
        continue;
      }
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
      $margin = ($item['bHi'] - $item['bLo']) / 2;

      $data[] = array(
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
   */
  protected function loadConversionReportData() {

    // All goals - summary report is always loaded in order to get the overview
    // data for the campaign.
    if (!isset($this->loadedReports['conversion'])) {
      $this->loadConversionReportHelper('confidence', TRUE);
    }
    $config = $this->getReportConfiguration();
    // All goals conversion report.
    if (empty($config['goal'])) {
      // All goals - detail report.
      if (!isset($this->loadedReports['conversion_detail'])) {
        $this->loadConversionReportHelper('confidence_detail', FALSE);
      }
      return;
    }
    // Load the detail and summary reports for the specified goal.
    $actions = visitor_actions_get_actions();

    $goal_id = $config['goal'];
    if (!isset($this->loadedReports['conversion_goals'][$goal_id])) {
      $this->loadedReports['conversion_goals'][$goal_id]['name'] = isset($actions[$goal_id]) ? $actions[$goal_id]['label'] : $goal_id;
      // Summary report.
      $this->loadConversionReportHelper('confidence_goal', TRUE);
      // Detail report.
      $this->loadConversionReportHelper('confidence_goal_detail', FALSE);
    }
  }

  /**
   * Extracts data from the raw aggregate confidence report that is prepared for
   * use within the report rendering process.
   *

   * @return array
   *   An associative array with information about the performance of each choice.
   */
  protected function extractConversionSummaryData() {
    $config = $this->getReportConfiguration();
    $report_name = 'confidence';
    if (!empty($config['goal'])) {
      $report_name = 'confidence_goal';
    }
    $report = $this->loadedReports['raw'][$report_name];
    if (!isset($report['data']) || empty($report['data']['items'])) {
      return array();
    }
    $items = $report['data']['items'];
    $data = array();
    $confidence = FALSE;
    $winner = '';
    $winning_value = NAN;
    foreach ($items as $i => $item) {
      if ($item['feature'] !== AcquiaLiftAPI::NO_FEATURES) {
        continue;
      }
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
      $margin = ($item['bHi'] - $item['bLo']) / 2;
      if ($item['signif']) {
        if (is_nan($winning_value) || $item['confidence'] > $winning_value) {
          $winning_value = $item['confidence'];
          $winner = $i;
          $confidence = TRUE;
        }
      }

      $data[$choice_id] = array(
        'counter' => $i,
        'choice_id' => $choice_id,
        'raw_label' => $option_id,
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
   * Loads all of the data necessary to generate the reports for the agent.
   */
  protected function loadReportData() {
    $this->loadConversionReportData();
    // Agent status
    if (!isset($this->loadedReports['status'])) {
      $this->loadAgentStatusData();
      if (isset($this->loadedReports['raw']['status']['error'])) {
        $this->loadedReports['status'] = FALSE;
      }
      else {
        $this->loadedReports['status'] = $this->extractOverviewReportData();
      }
    }
    // Confidence report data.
    if (!isset($this->loadedReports['confidence'])) {
      $this->loadConfidenceData('confidence');
      if (isset($this->loadedReports['raw']['confidence']['error'])) {
        $this->loadedReports['confidence'] = FALSE;
      }
      else {
        $this->loadedReports['confidence'] = $this->extractConfidenceReportData();
      }
    }

    if (!isset($this->loadedReports['extracted_total'])) {
      // Combine data to form all campaign report data.
      $this->extractCampaignReportData();
    }
  }

  /**
   * Alter the report data returned from API calls to combine into data that is
   * ready for presentation within individual campaign reports.
   *
   * @see loadReportData()
   * @see extractOverviewReportData()
   * @see extractConfidenceReportData()
   * @see extractPotentialTargetingValues()
   */
  protected function extractCampaignReportData() {
    $this->loadedReports['extracted_total'] = TRUE;
    if (!is_array($this->loadedReports['status']) || !is_array($this->loadedReports['confidence'])) {
      return;
    }

    // Determine overall confidence based on confidence in choices.
    $total_confident = 0;
    if (isset($this->loadedReports['confidence']['variations'])) {
      foreach ($this->loadedReports['confidence']['variations'] as $choice) {
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
    foreach($this->loadedReports['status'] as &$report) {
      $report['total_lift_positive'] = $report['unformatted']['total_lift'] > self::LIFT_THRESHHOLD && $total_confident > 0;
      $report['total_confident'] = $total_confident;
      $report['confidence_level'] = $total_confident > 0 ? 'high' : 'low';
      $report['time_running'] = isset($interval) ? $interval->format('%mm, %dd') : '1d';
    }

    // Build the experiment report data
    $this->loadedReports['experiment']['choices'] = isset($this->loadedReports['confidence']['variations']) ? $this->loadedReports['confidence']['variations'] : array();
  }

  /**
   * Extracts the required confidence report data from the items returned by Acquia Lift.
   *
   * @param $items
   *   An array of items as return from Acquia Lift.
   * @return array
   *   An associative array with information about the performance of each choice.
   */
  protected function extractConfidenceReportData() {
    $report = $this->loadedReports['raw']['confidence'];
    if (!isset($report['data']) || empty($report['data']['items'])) {
      return array();
    }
    $items = $report['data']['items'];
    $data = array(
      'point' => $items[0]['point'],
      'variations' => array(),
      'goal_value_differential' => FALSE,
    );
    $counter = 1;
    foreach ($items as $i => $item) {
      if ($item['feature'] != AcquiaLiftAPI::NO_FEATURES) {
        continue;
      }
      $counter++;
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
      $data['variations'][$choice] = array(
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

  protected  function getSessionCount() {
    $status_report_data = $this->getStatusReportItems();
    return $this->formatReportNumber($status_report_data['totals']['sessions']['count']);
  }

  protected function getGoalCount() {
    $status_report_data = $this->getStatusReportItems();
    return $this->formatReportNumber($this->formatReportNumber($status_report_data['totals']['goals']['count']));
  }
}
