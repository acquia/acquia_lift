<?php
/**
 * @file
 * Provides an agent type for Acquia Lift
 */

interface AcquiaLiftAgentInterface {


  /**
   * Organizes an array of option sets into decision points.
   *
   * @param $option_sets
   *   An array of option sets.
   *
   * @return array
   *   An associative array keyed by decision points, whose values are associative
   *   arrays with decision names as keys and an array of choices as values.
   */
  public static function convertOptionSetsToDecisions($option_sets);

  /**
   * Retrieves a conversion report.
   *
   * @param $options
   *   An array of report options.
   *   - decision_name: (Optional) The decision to get conversion report for.
   *   - goal: (Optional) The goal for limiting the report, defaults to all goals.
   *   - start: (Optional) The start time for report data in Y-m-d format.
   *     Defaults to agent start.
   *   - end: (Optional) The end time for report data in Y-m-d format.  Defaults
   *     to the current date.
   * @return array
   *   A renderable array.
   */
  public function buildConversionReport($options);
}

interface AcquiaLiftReportInterface {
  /**
   * Get the confidence measure for determining the statistical significance
   * of tests.
   */
  public function getConfidenceMeasure();

  /**
   * Set the confidence measure for determining the statistical significance
   * of tests.
   *
   * @param $value
   *   The value as a number between 0 and 100.
   */
  public function setConfidenceMeasure($value);

  /**
   * Retrieves a conversion report.
   *
   * @param $options
   *   An array of report options.
   *   - decision_name: (Optional) The decision to get conversion report for.
   *   - goal: (Optional) The goal for limiting the report, defaults to all goals.
   *   - start: (Optional) The start time for report data in Y-m-d format.
   *     Defaults to agent start.
   *   - end: (Optional) The end time for report data in Y-m-d format.  Defaults
   *     to the current date.
   * @return array
   *   A renderable array.
   */
  public function buildConversionReport($options);
}

/**
 * Factory class to create a report object for Acquia Lift.
 */
class AcquiaLiftReportFactory {

  /**
   * Creates an instance of the Acquia Lift Report class.
   *
   * @param $agent_instance
   *   The Acquia Lift agent that the reporting is for.
   * @param $api_instance
   *   The Acquia Lift API classes in use.
   * @return
   *   The AcquiaLiftReport class to use.
   */
  public static function create(PersonalizeAgentInterface $agent_instance, $api_instance, $legacy = FALSE) {
    $agent_name = $agent_instance->getMachineName();
    // Check if this agent is set up to read reports from a file instead of
    // calling the API.
    $report_file = variable_get("acquia_lift_report_source_$agent_name", '');
    if (!empty($report_file)) {
      $report_source = new AcquiaLiftReportDataFromFile($report_file, new AcquiaLiftReportCache());
    }
    else {
      $report_source = $api_instance;
    }

    $class = $legacy ? 'AcquiaLiftReportLegacy' : 'AcquiaLiftReport';
    $report = new $class($agent_instance, $report_source);
    $report->setConfidenceMeasure(variable_get('acquia_lift_confidence_measure', 95));
    return $report;
  }
}

class AcquiaLiftAgent extends PersonalizeAgentBase implements PersonalizeAgentGoalInterface, AcquiaLiftAgentInterface, PersonalizeAgentReportInterface {

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
   * An instance of PersonalizeAgentReportInterface.
   *
   * The agent acts as a facade to the reporting class for reporting requests.
   *
   * @var PersonalizeAgentReportInterface
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
  protected function getReporting($legacy = FALSE) {
    if (empty($this->reporting)) {
      $this->reporting = AcquiaLiftReportFactory::create($this, $this->liftAPI, $legacy);
    }
    return $this->reporting;
  }

  /**
   * Implements PersonalizeAgentReportInterface::renderStatsForOptionSet().
   *
   * A facade to the reporting class.
   */
  public function renderStatsForOptionSet($option_set, $date_from, $date_to = NULL) {
    return $this->getReporting()->renderStatsForOptionSet($option_set, $date_from, $date_to);
  }

  /**
   * Implements PersonalizeAgentReportInterface::buildCampaignReports().
   *
   * A facade to the reporting class.
   */
  public function buildCampaignReports($options) {
    $legacy = FALSE;
    if (!empty($options['legacy'])) {
      $legacy = TRUE;
      unset($options['legacy']);
    }
    return $this->getReporting($legacy)->buildCampaignReports($options);
  }

  /**
   * Implements AcquiaLiftReportInterface()::buildConversionReport().
   */
  public function buildConversionReport($options) {
    return $this->getReporting()->buildConversionReport($options);
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
class AcquiaLiftReport implements PersonalizeAgentReportInterface, AcquiaLiftReportInterface {

  /**
   * The value to show when report data is not applicable.
   */
  const DATA_NA = '&mdash;';

  /**
   * The threshold to use above which lift percentage will be positively noted.
   */
  const LIFT_THRESHHOLD = 0;

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
   * Constructs an AcquiaLiftReport object
   *
   * @param PersonalizeAgentInterface $agent
   *   The agent the report is for.
   *
   * @param AcquiaLiftReportDataSourceInterface $report_data_src
   *   The source for the report data.
   */
  function __construct(PersonalizeAgentInterface $agent, AcquiaLiftReportDataSourceInterface $report_data_src) {
    $this->agent = $agent;
    $this->reportDataSrc = $report_data_src;
  }

  /**
   * Implements AcquiaLiftReportInterface()::getConfidenceMeasure().
   */
  public function getConfidenceMeasure() {
    return $this->confidence_measure;
  }

  /**
   * Implements AcquiaLiftReportInterface()::setConfidenceMeasure().
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
    $today_only = $date_from === date('Y-m-d') && empty($date_to);
    $date_from = empty($date_from) ? date('Y-m-d', $this->agent->getStartTime()) : $date_from;
    $date_to = empty($date_to) ? date('Y-m-d') : $date_to;

    $confidence_measure = $this->getConfidenceMeasure();
    // Convert the confidence measure from a percentage to a value between
    // 0 and 1 as expected by the Lift API.
    $confidence_measure /= 100;

    // Set the configuration for the reports.
    $this->reportConfig = array(
      'today_only' => $today_only,
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
   * Implements AcquiaLiftReportInterface()::buildConversionReport().
   */
  public function buildConversionReport($options) {
    $this->setReportConfiguration($options);
    $report_name = t('All goals');
    $goal = NULL;
    if (!empty($options['goal'])) {
      $goal = $options['goal'];
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
    $this->loadConversionReportHelper('confidence_detail', FALSE, $goal);
    $this->loadConversionReportHelper('confidence', TRUE, $goal);

    if ($this->loadedReports['conversion_all'] == FALSE) {
      return array(
        '#markup' => t('There was a problem generating the conversion report.'),
      );
    }

    $reports = $this->buildConversionReports(array(
      'name' => $report_name,
      'detail' => empty($goal) ? $this->loadedReports['conversion_all']['detail'] : $this->loadedReports['conversion_goals'][$goal]['detail'],
      'summary' => empty($goal) ? $this->loadedReports['conversion_all']['summary'] : $this->loadedReports['conversion_goals'][$goal]['summary'],
    ), $this->loadedReports);
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
   * Builds the conversion reports to show basic conversion metrics for report
   * requested in the report_data.
   *
   *   The render array for the report.
   */
  protected function buildAllConversionReports() {
    $config = $this->getReportConfiguration();
    if (empty($config['goal'])) {
      // Generate the conversion reports for all goals.
      $reports = $this->buildConversionReports(array(
        'name' => t('All goals'),
        'detail' => $this->loadedReports['conversion_all']['detail'],
        'summary' => $this->loadedReports['conversion_all']['summary'],
      ), $this->loadedReports);
    }
    else {
      // Generate the conversion reports for the specified goal.
      $reports = $this->buildConversionReports($this->loadedReports['conversion_goals'][$config['goal']], $this->loadedReports);
    }
    if ($reports == FALSE) {
      drupal_set_message(t('There was a problem retrieving the report data. Please try again later.'), 'error');
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
    $config = $this->getReportConfiguration();
    $status_report = $this->loadedReports['raw']['status'];
    if (!isset($status_report['data']) || !isset($status_report['data'][$config['machine_name']])) {
      return array();
    }
    $items = $status_report['data'][$config['machine_name']];
    $agent_data = $this->agent->getData();
    $decision_style = $agent_data['decision_style'] === 'adaptive' ? t('Auto-personalize') : t('A/B');
    $total_variations = isset($agent_data['decisions']) ? count($agent_data['decisions']) : 0;
    $report = array();
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
   * @param $report_name
   *   The name of the raw report to extract data from.
   * @return array
   *   An associative array with information about the performance of each choice.
   */
  protected function extractConversionReportData($report_name) {
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
      $margin = (($item['bHi'] - $item['bLo'])/2) * 100;

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
   * @param $report_name
   *   The name of the raw report to extract data from.
   * @return array
   *   An associative array with information about the performance of each choice.
   */
  protected function extractConversionSummaryData($report_name) {
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
      $margin = (($item['bHi'] - $item['bLo'])/2);

      if ($item['signif']) {
        if (is_nan($winning_value) || $item['confidence'] > $winning_value) {
          $winning_value = $item['lift']['default'];
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
   * Builds the render array for the metrics portion of the report.
   *
   * @param array $report_data
   *   All of the reporting data for this AB report.
   * @param array|bool
   *   A render array for the report or FALSE if it cannot be generated.
   */
  protected function buildConversionDetailReport($report_data) {
    if ($report_data === FALSE) {
      return FALSE;
    }
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
      t('Margin of error (%)'),
    );
    $rows = array();
    foreach ($report_data as $choice_data) {
      $rows[] = array(
        'data' => array(
          array(
            'data' => $choice_data['timestamp'],
          ),
          array(
            'data' => $choice_data['choice_id'],
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
   * @param array $report_data
   *   Reporting data for this summary report.
   * @param bool $confidence
   *   Whether the report has confidence.
   * @param bool $winner
   *   The index of the winning option or NULL if there's no winner
   * @param array|bool
   *   A render array for the report or FALSE if it cannot be generated.
   */
  protected function buildConversionSummaryReport($report_data, $confidence, $winner) {
    if ($report_data === FALSE) {
      return FALSE;
    }
    $config = $this->getReportConfiguration();
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
    $num_rows = count($report_data['data']);
    foreach ($report_data['data'] as $choice_id => $choice_data) {
        $row_data = array(
          array(
            'data' => $choice_id,
            'data-acquia-lift-variation-label' => $this->getVariationLabel($choice_data['counter'], $choice_data['control']),
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
        if (empty($rows) && $report_data['overview']['confidence'] === FALSE) {
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
          $row_data[] = $confidence && $winner == $choice_data['counter'] ? '<span class="lift-winner">' . t('Winner') . '</span>' : '';
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
   * Build a set of confidence reports from the report data.
   *
   * @param array $report_data
   *   The extracted report data with the following keys:
   *   - name: The name of the section
   *   - detail:  The detail report data
   *   - summary: The summary report data
   * @return array|bool
   *   The render array for the reports or false if invalid data.
   */
  protected function buildConversionReports($report_data) {
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
    $build['reports']['detail'] = $this->buildConversionDetailReport($report_data['detail']);
    $confidence = $this->loadedReports['conversion_all']['summary']['overview']['confidence'];
    $winner = $this->loadedReports['conversion_all']['summary']['overview']['winner'];
    $build['reports']['summary'] = $this->buildConversionSummaryReport($report_data['summary'], $confidence, $winner);
    $build['reports']['#theme_wrappers'] = array('container');
    $build['reports']['#attributes'] = array('class' => array('lift-statistic-category'));
    return $build;
  }

  /**
   * Implements PersonalizeAgentReportInterface::renderStatsForOptionSet().
   */
  public function renderStatsForOptionSet($option_set, $date_from, $date_to = NULL) {
    $date_start = date('Y-m-d', $date_from);
    if (empty($date_to)) {
      $date_to = time();
    }
    $date_end = date('Y-m-d', $date_to);
    $this->setReportConfiguration(array(
      'start' => $date_start,
      'end' => $date_end,
    ));
    $this->loadConfidenceData('confidence');
    if (isset($this->loadedReports['raw']['confidence']['error']) || !isset($this->loadedReports['raw']['confidence']['data'])) {
      return array();
    }
    else {
      $data = $this->extractConfidenceReportData();
    }

    $decisions = $goals = 0;
    // Get a total count of all decisions made for this Option Set and all
    // goals received.
    foreach ($data['variations'] as $choice => $info) {
      list($decision_name, $option_id) = explode(':', $choice);
      if ($decision_name != $option_set->decision_name) {
        continue;
      }
      $decisions += $info['decisions'];
      $goals += $info['goals'];
    }
    $report[] = format_plural($decisions, '1 view', '@count views');
    $report[] = format_plural($goals, '1 goal', '@count goals');
    return $report;
  }

  /**
   * Implements PersonalizeAgentReportInterface::buildCampaignReports().
   */
  public function buildCampaignReports($options) {
    $this->setReportConfiguration($options);
    $this->loadReportData();
    $goal = !empty($options['goal']) ? $options['goal'] : NULL;
    $reports = array(
      'overview' => $this->buildOverviewReport($goal),
      'experiment' => $this->buildAllConversionReports($goal),
    );
    $reports['#has_data'] = isset($reports['overview']['shown']['#title']) ? $reports['overview']['shown']['#title'] > 0 : FALSE;

    if (!is_array($this->loadedReports['status']) || !is_array($this->loadedReports['confidence'])) {
      drupal_set_message(t('There was a problem retrieving the report data.  Please try again later.'), 'error');
    }
    else if ($reports['#has_data'] && $this->loadedReports['status']['all']['total_confident'] == 0) {
      drupal_set_message($this->getLowConfidenceMessage(), 'warning');
    }
    return $reports;
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
   * Returns a render array representing the overview report for the given dates.
   *
   * @return array
   *   A render array representing the overview report.
   */
  protected function buildOverviewReport() {
    $report = $this->loadedReports['status'];
    if ($report === FALSE) {
      return array();
    }
    $config = $this->getReportConfiguration();
    if ($config['today_only']) {
      if (!isset($report['today'])) {
        return array();
      }
      $overview_report = $report['today'];
    }
    else {
      if (!isset($report['all'])) {
        return array();
      }
      $overview_report = $report['all'];
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

}
