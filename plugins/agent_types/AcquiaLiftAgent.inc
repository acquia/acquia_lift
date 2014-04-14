<?php
/**
 * @file
 * Provides an agent type for Acquia Lift
 */

interface AcquiaLiftAgentInterface {
  /**
   * Synchronizes the decision structure for this agent to Acquia Lift.
   *
   * If decision names or decision points have been renamed or removed, they
   * need to be deleted in Acquia Lift and the new points and decisions
   * added.
   *
   * @param $old_decisions
   *   Array representing the old decision structure from when it was
   *   last sync'd to AcquiaLift.
   * @param $new_decisions
   *   Array representing the new decision structure.
   * @return mixed
   */
  public function syncDecisions($old_decisions, $new_decisions);

  /**
   * Synchronize the goals for this agent to Acquia Lift.
   *
   * @param $old_goals
   *   The old goals that Acquia Lift knows about, with goal names as
   *   keys and goal values as values.
   * @param $new_goals
   *   The new goals, with goal names as keys and goal values as values.
   */
  public function syncGoals($old_goals, $new_goals);

  /**
   * Syncs fixed targeting rules to Acquia Lift.
   *
   * @param $option_sets
   *   An array of option set objects whose options may have fixed targeting rules
   *   associated with them.
   */
  public function syncFixedTargeting($option_sets);

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

}

class AcquiaLiftAgent extends PersonalizeAgentBase implements PersonalizeAgentGoalInterface, AcquiaLiftAgentInterface, PersonalizeExplicitTargetingInterface, PersonalizeAgentReportInterface {

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

  /**
   * An instance of DrupalQueueInterface
   *
   * @var DrupalQueueInterface
   */
  protected $queue;

  /**
   * Implements PersonalizeAgentInterface::create().
   */
  public static function create($agent_data) {
    try {
      $acquia_lift_api = AcquiaLiftAPI::getInstance(variable_get('acquia_lift_account_info', array()));
      return new self($agent_data->machine_name, $agent_data->label, $agent_data->data, $agent_data->status, !empty($agent_data->started) ? $agent_data->started : NULL, $acquia_lift_api);
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
  public function __construct($machine_name, $title, $data, $status, $started, AcquiaLiftAPI $acquia_lift_api) {
    parent::__construct($machine_name, $title, $data, $status, $started = NULL);
    $this->liftAPI = $acquia_lift_api;
    $this->reporting = new AcquiaLiftReport($this, $this->liftAPI);
  }

  public function getType() {
    return 'acquia_lift';
  }

  /**
   * Implements PersonalizeAgentInterface::getAssets().
   */
  public function getAssets() {
    return array(
      'js' => array(
        array(
          'type' => 'setting',
          'data' => array(
            'acquia_lift' => array(
              'apiKey' => $this->liftAPI->getApiKey(),
              'owner' => $this->liftAPI->getOwnerCode(),
              'baseUrl' => $this->liftAPI->getApiUrl(),
              'featureStringReplacePattern' => AcquiaLiftAPI::FEATURE_STRING_REPLACE_PATTERN,
              'featureStringMaxLength' => AcquiaLiftAPI::FEATURE_STRING_MAX_LENGTH,
              // @todo Add support for mutually exclusive values.
              'featureStringSeparator' => AcquiaLiftAPI::FEATURE_STRING_SEPARATOR_NONMUTEX,
            )
          )
        ),
        // Add the Acquia Lift API js wrapper and the Acquia Lift integration js.
        drupal_get_path('module', 'acquia_lift') . '/js/acquia-lift-js-wrapper-min.js' => array('type' => 'file', 'scope' => 'header'),
        drupal_get_path('module', 'acquia_lift') . '/js/acquia_lift.js' => array('type' => 'file', 'scope' => 'footer', 'defer' => TRUE),
      )
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
   * Implements PersonalizeAgentInterface::optionsForm().
   */
  public static function optionsForm($agent_data) {
    $account_info = variable_get('acquia_lift_account_info', array());
    if (empty($account_info)) {
      drupal_set_message(t('Your Acquia Lift account info has not been configured. Any Acquia Lift campaigns you create here will not work until you configure your account info !here', array('!here' => l('here', 'admin/config/content/personalize/acquia-lift'))), 'error');
    }
    $form = array();
    $form['#attached'] = array(
      'css' => array(
        drupal_get_path('module', 'acquia_lift') . '/css/personalize_acquia_lift_admin.css',
        drupal_get_path('module', 'acquia_lift') . '/css/acquia_lift.admin.css',
      ),
      'js' => array(
        drupal_get_path('module', 'acquia_lift') . '/js/acquia_lift.agent.admin.js',
      ),
    );
    $option_parents = array('agent_basic_info', 'options', 'acquia_lift');
    $control_rate = isset($agent_data->data['control_rate']) ? $agent_data->data['control_rate'] : 10;
    $form['control'] = array(
      '#type' => 'fieldset',
      '#tree' => FALSE,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#title' => t('Control (@control_rate%)', array('@control_rate' => $control_rate)),
    );
    $control_rate_parents = $option_parents;
    $control_rate_parents[] = 'control_rate';
    $form['control']['control_rate'] = array(
      '#type' => 'acquia_lift_percentage',
      '#parents' => $control_rate_parents,
      '#title' => t('Control Group'),
      '#field_suffix' => '%',
      '#size' => 3,
      '#description' => t('A fixed baseline variation will be shown, by default the first variation in the set.'),
      '#default_value' => $control_rate,
      '#rest_title' => t('Test Group'),
      '#rest_description' => t('Personalized variations will be shown.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['decision_style'] = array(
      '#type' => 'radios',
      '#title' => t('Decision Style'),
      '#options' => array('adaptive' => t('Auto-personalize'), 'random' => t('A/B test')),
      '#default_value' => isset($agent_data->data['decision_style']) ? $agent_data->data['decision_style'] : 'adaptive',
      '#title_display' => 'invisible',
    );
    $form['decision_style']['adaptive'] = array(
      '#description' => t('Adapts to users and chooses the best option over time.'),
    );
    $form['decision_style']['random'] = array(
      '#description' => t('Tests variations and reports results.'),
    );
    $explore_rate = isset($agent_data->data['explore_rate']) ? $agent_data->data['explore_rate'] : 20;
    $form['distribution'] = array(
      '#type' => 'fieldset',
      '#tree' => FALSE,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#title' => t('Distribution (@explore_rate/@rest)', array(
        '@explore_rate' => $explore_rate,
        '@rest' => (100 - $explore_rate),
      )),
      '#states' => array(
        'visible' => array(
          ':input[name="agent_basic_info[options][acquia_lift][decision_style]"]' => array('value' => 'adaptive'),
        ),
      ),
    );
    $explore_rate_parents = $option_parents;
    $explore_rate_parents[] = 'explore_rate';
    $form['distribution']['explore_rate'] = array(
      '#type' => 'acquia_lift_percentage',
      '#parents' => $explore_rate_parents,
      '#title' => t('Random Group'),
      '#field_suffix' => '%',
      '#description' => t('Variations will be shown randomly and tracked to adjust for false positives.'),
      '#size' => 3,
      '#default_value' => isset($agent_data->data['explore_rate']) ? $agent_data->data['explore_rate'] : 20,
      '#rest_title' => t('Personalized Group'),
      '#rest_description' => t('The "best" variation will be shown for each visitor based on our algorithm.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    return $form;
  }

  /**
   * Implements PersonalizeAgentInterface::optionsFormValidate().
   */
  public static function optionsFormValidate($form, &$form_state) {
    $values = $form_state['values']['agent_basic_info'];
    if (isset($values['options']['acquia_lift']) && isset($values['options']['acquia_lift']['control_rate'])) {
      $rate = $values['options']['acquia_lift']['control_rate'];
      if (!is_numeric($rate) || !($rate >= 0 && $rate <= 100)) {
        form_set_error('agent_basic_info][options][acquia_lift][control_rate', t('Invalid percent to test specified'));
      }
    }
    if (isset($values['options']['acquia_lift']) && isset($values['options']['acquia_lift']['explore_rate'])) {
      $rate = $values['options']['acquia_lift']['explore_rate'];
      if (!is_numeric($rate) || !($rate >= 0 && $rate <= 100)) {
        form_set_error('agent_basic_info][options][acquia_lift][explore_rate', t('Invalid percent to test specified'));
      }
    }
  }

  /**
   * Implements PersonalizeAgentInterface::postSave().
   */
  public function postSave($old_data) {
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
        'enabled',
        $acquia_lift_control_rate,
        $acquia_lift_explore_rate,
      ),
    );
    $acquia_lift_context_needs_deleting = isset($old_data->data['visitor_context']['acquia_lift_context']);
    if (isset($this->data['visitor_context']['acquia_lift_context'])) {
      // Whereas non-Acquia Lift visitor_context plugins operate by adding extra
      // info at the time of getting a decision, Acquia Lift targeting needs to
      // be set up on the Acquia Lift side before any decisions are made.
      $auto_targeting = array_filter($this->data['visitor_context']['acquia_lift_context']);
      if (!empty($auto_targeting)) {
        $acquia_lift_context_needs_deleting = FALSE;
        // Add an item for saving the targeting rule.
        $items[] = array(
          'method' => 'saveAutoTargetingRule',
          'args' => array(
            $this->machineName,
            array_keys($auto_targeting)
          ),
        );
      }
    }
    if ($acquia_lift_context_needs_deleting) {
      // Acquia Lift may have a targeting rule for this agent, so we need
      // to delete it.
      $items[] = array(
        'method' => 'deleteAutoTargetingRule',
        'args' => array(
          $this->machineName
        ),
      );
    }
    $this->queueItems($items);
  }

  /**
   * Implements PersonalizeExplicitTargetingInterface::explicitTargetingSupportMultiple().
   */
  public static function explicitTargetingSupportMultiple() {
    return PersonalizeExplicitTargetingInterface::EXPLICIT_TARGETING_MULTIPLE_BOTH;
  }

  /**
   * Implements PersonalizeAgentInterface::convertContextToFeatureString().
   *
   * This is essentially a PHP version of the js in the convertContextToFeatureString
   * function in acquia_lift.js
   */
  public static function convertContextToFeatureString($name, $value, $is_mutex = FALSE) {
    $separator = $is_mutex ? AcquiaLiftAPI::FEATURE_STRING_SEPARATOR_MUTEX : AcquiaLiftAPI::FEATURE_STRING_SEPARATOR_NONMUTEX;
    $prefix_max_length = floor((AcquiaLiftAPI::FEATURE_STRING_MAX_LENGTH - strlen($separator)) / 2);

    $prefix = AcquiaLiftAPI::cleanFeatureString($name);
    $value = AcquiaLiftAPI::cleanFeatureString($value);
    // Make a string of the visitor context item in the format Acquia Lift can
    // consume.
    $feature_string = $prefix . $separator . $value;

    while (strlen($feature_string) > AcquiaLiftAPI::FEATURE_STRING_MAX_LENGTH) {
      // Acquia Lift has a hard character limit for feature strings.
      if (strlen($prefix) > $prefix_max_length) {
        // Start by truncating the prefix down to half the max length.
        $prefix = substr($prefix, 0, $prefix_max_length);
        $feature_string = $prefix . $separator . $value;
      }
      else {
        // Otherwise just truncate the whole thing down to the max length.
        $feature_string = substr($feature_string, 0, AcquiaLiftAPI::FEATURE_STRING_MAX_LENGTH);
      }
    }
    return $feature_string;
  }

  /**
   * Implements PersonalizeAgentReportInterface::renderStatsForOptionSet().
   *
   * A facade to the reporting class.
   */
  public function renderStatsForOptionSet($option_set, $date_from, $date_to = NULL) {
    return $this->reporting->renderStatsForOptionSet($option_set, $date_from, $date_to);
  }

  /**
   * Implements PersonalizeAgentReportInterface::buildCampaignReports().
   *
   * A facade to the reporting class.
   */
  public function buildCampaignReports($decision_name = NULL, $date_from = NULL, $date_to = NULL) {
    return $this->reporting->buildCampaignReports($decision_name, $date_from, $date_to);
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
   * Implements PersonalizeAgentInterface::errors().
   */
  public function errors() {
    $errors = array();
    $acquia_lift_agent = $this->liftAPI->getAgent($this->machineName);
    if (!$acquia_lift_agent || $acquia_lift_agent['status'] !== 'enabled') {
      $errors[] = t('The status of the Acquia Lift agent is @status', array('@status' => $acquia_lift_agent['status']));
    }
    // Make sure Acquia Lift knows about the agent's goals.
    $goals = personalize_goal_load_by_conditions(array('agent' => $this->machineName));
    $acquia_lift_goals = $this->liftAPI->getGoalsForAgent($this->machineName);
    if (empty($goals)) {
      // Acquia Lift agents need goals.
      $errors[] = t('No goals have been set up for this agent');
    }

    $discrepancies = FALSE;
    foreach($goals as $goal) {
      if (!in_array($goal['action'], $acquia_lift_goals)) {
        $errors[] = t('Goal @goal has not been sync\'d to the Acquia Lift agent.', array('@goal' => $goal['action']));
        $discrepancies = TRUE;
      }
    }
    // Make sure all decision points are known by Acquia Lift.
    $option_sets = personalize_option_set_load_by_agent($this->machineName);
    $decision_tree = self::convertOptionSetsToDecisions($option_sets);
    $acquia_lift_points = $this->liftAPI->getPointsForAgent($this->machineName);
    foreach ($decision_tree as $point => $decisions) {
      if (!in_array($point, $acquia_lift_points)) {
        $errors[] = t('Point @point has not been sync\'d to the Acquia Lift agent.', array('@point' => $point));
        $discrepancies = TRUE;
      }
      $acquia_lift_decisions = $this->liftAPI->getDecisionsForPoint($this->machineName, $point);
      foreach ($decisions as $decision_name => $options) {
        if (!in_array($decision_name, $acquia_lift_decisions)) {
          $errors[] = t('Decision @decision has not been sync\'d to the Acquia Lift agent.', array('@decision' => $decision_name));
          $discrepancies = TRUE;
        }
        $acquia_lift_choices = $this->liftAPI->getChoicesForDecision($this->machineName, $point, $decision_name);
        foreach ($options as $option) {
          if (!in_array($option, $acquia_lift_choices)) {
            $errors[] = t('Option @choice has not been sync\'d to the Acquia Lift agent.', array('@choice' => $option));
            $discrepancies = TRUE;
          }
        }
      }
      // @todo Check the fixed targeting for each decision point.
    }
    if ($discrepancies) {
      // Add a general message about how to resolve discrepancies.
      $message = t('To resolve the discrepancies between your agent configuration here and what has been sync\'d to the Acquia Lift service, try saving your campaign and its variation sets and goals again.');
      if (user_access('administer site configuration')) {
        $message .= t(' If that still does not resolve it, try <a href="@cron">running cron</a>.', array('@cron' => url('admin/reports/status/run-cron')));
      }
      $errors[] = $message;
    }
    return $errors;
  }

  /**
   * Returns a queue to use.
   *
   * @return DrupalQueueInterface
   */
  protected function getQueue() {
    if ($this->queue !== NULL) {
      return $this->queue;
    }
    return DrupalQueue::get('acquia_lift_sync');
  }

  /**
   * Sets the queue to use.
   *
   * @param DrupalQueueInterface $queue
   */
  public function setQueue(DrupalQueueInterface $queue) {
    $this->queue = $queue;
  }

  /**
   * Implements AcquiaLiftAgentInterface::syncDecisions().
   */
  public function syncDecisions($old_decisions, $new_decisions) {
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
    $this->queueItems($items);
  }

  /**
   * Implements AcquiaLiftAgentInterface::syncGoals().
   */
  public function syncGoals($old_goals, $new_goals) {
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
    $this->queueItems($items);
  }

  /**
   * Implements AcquiaLiftAgentInterface::syncFixedTargeting().
   */
  public function syncFixedTargeting($option_sets) {
    $items = array();
    // If any of this agent's option sets has explicit targeting mappings configured,
    // we need to send these mappings to Acquia Lift.
    $mappings = array();
    foreach ($option_sets as $option_set) {
      $point_name = $option_set->decision_point;
      $decision_name = $option_set->decision_name;
      $mappings[$point_name] = isset($mappings[$point_name]) ? $mappings[$point_name] : array();
      foreach ($option_set->options as $option) {
        if (isset($option['fixed_targeting'])) {
          // Check if we're supposed to AND or OR mulitple features together.
          if (isset($option['fixed_targeting_strategy']) && $option['fixed_targeting_strategy'] == 'AND') {
            // Create a single mapping, with a comma-separated list of features.
            $mappings[$point_name][] = array(
              'feature' => implode(',', $option['fixed_targeting']),
              'decision' => $decision_name . ':' . $option['option_id']
            );
          }
          else {
            // Create a mapping for each feature and they will be OR'd together.
            foreach ($option['fixed_targeting'] as $feature) {
              $mappings[$point_name][] = array(
                'feature' => $feature,
                'decision' => $decision_name . ':' . $option['option_id']
              );
            }
          }
        }
      }
    }

    // Send mappings per decision point.
    foreach ($mappings as $point_name => $map) {
      $items[] = array(
        'method' => 'saveFixedTargetingMapping',
        'args' => array(
          $this->machineName,
          $point_name,
          $map
        )
      );
    }
    $this->queueItems($items);
  }

  /**
   * Adds items to the queue and sets a message to inform the user.
   *
   * @param $items
   *   An array of items to add to the queue.
   */
  protected function queueItems($items) {
    if (!empty($items)) {
      foreach ($items as $item) {
        // Create a hash of the item. This will prevent duplicate items from
        // being added to the queue.
        $hash = md5(serialize($item));
        $item['hash'] = $hash;
        $this->getQueue()->createItem($item);
      }
      // Make sure the queue gets triggered on the next request.
      $_SESSION['acquia_lift_queue_trigger'] = 1;
    }
  }
}

/**
 * Responsible retrieving and generating reports on the Acquia Lift agent.
 */
class AcquiaLiftReport implements PersonalizeAgentReportInterface {

  /**
   * The threshold to use above which lift percentage will be positively noted.
   */
  const LIFT_THRESHHOLD = 0;

  /**
   * The threshold to use above which stability will be positively noted.
   */
  const STABILITY_THRESHOLD = 25;

  /**
   * The value to show when report data is not applicable.
   */
  const DATA_NA = '&mdash;';

  /**
   * The value representing no features applied to an experiment.
   */
  const NO_FEATURES = '(none)';

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

  function __construct($agent, $liftAPI) {
    $this->agent = $agent;
    $this->liftAPI = $liftAPI;
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

    try {
      $confidenceReport = $this->liftAPI->getConfidenceReport($this->agent->getMachineName(), $date_start, $date_end, $option_set->decision_point, array(self::NO_FEATURES));
      if (!isset($confidenceReport['data']['items'])) {
        return array();
      }
      $data = $this->extractConfidenceReportData($confidenceReport['data']['items']);
      $decisions = $goals = 0;
      // Get a total count of all decisions made for this Option Set and all
      // goals received.
      foreach ($data['features'][self::NO_FEATURES] as $choice => $info) {
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
    catch (AcquiaLiftException $e) {
      // The agent probably hasn't been enabled yet on the Acquia Lift end, just
      // return an empty report.
      return array();
    }
  }

  /**
   * Implements PersonalizeAgentReportInterface::buildCampaignReports().
   */
  public function buildCampaignReports($decision_name = NULL, $date_from = NULL, $date_to = NULL) {
    $report_data = $this->loadReportData($decision_name, $date_from, $date_to);
    $reports = array(
      'overview' => $this->buildOverviewReport($report_data),
      'experiment' => $this->buildExperimentReport($report_data),
      'context' => $this->buildContextReport($report_data),
      'stability' => $this->buildStabilityReport($report_data),
      'targeting' => $this->buildReportContextSelection($report_data),
    );
    return $reports;
  }

  /**
   * Loads all of the data necessary to generate the reports for the agent.
   *
   * @param $decision_name
   *   (Optional) decision point name to limit results.
   * @param $date_from
   *   The starting date for report data in Y-m-d format, or null for agent
   *   start date.
   * @param $date_to
   *   The ending date for report data in Y-m-d format, or null for today.
   * @return array
   *   The reporting data for the date range as an array with keys for
   *   - status: The general agent status report data
   *   - confidence: The confidence report data
   *   - targeting: The targeting report data
   */
  protected function loadReportData($decision_name = NULL, $date_from = NULL, $date_to = NULL) {
    $report_data = &drupal_static(__FUNCTION__);
    $key = 'S' . $date_from . 'E' . $date_to;
    $machine_name = $this->agent->getMachineName();

    if (!isset($report_data[$key])) {
      $today_only = $date_from === date('Y-m-d') && empty($date_to);
      $date_from = empty($date_from) ? date('Y-m-d', $this->agent->started) : $date_from;
      $date_to = empty($date_to) ? date('Y-m-d') : $date_to;

      // Save basic report generation information with the data.
      $report_data[$key]['today_only'] = $today_only;
      $report_data[$key]['date_from'] = $date_from;
      $report_data[$key]['date_to'] = $date_to;
      $report_data[$key]['decision_name'] = $decision_name;

      // Get all context filtering values
      try {
        $contextFilters = $this->liftAPI->getPotentialTargetingValues($machine_name);
        $report_data[$key]['potential_context'] = $this->extractPotentialTargetingValues($contextFilters['data']);
      }
      catch (Exception $e) {
        $report_data[$key]['potential_context'] = FALSE;
      }

      // Agent status report data.
      try {
        if ($today_only) {
          $num_days = 1;
        }
        else {
          $interval = date_diff(date_create($date_from), date_create($date_to));
          $num_days = $interval->days;
        }
        $overviewReport = $this->liftAPI->getAgentStatusReport(array($machine_name), $num_days);
        $report_data[$key]['status'] = $this->extractOverviewReportData($overviewReport['data'][$machine_name]);
      }
      catch (AcquiaLiftException $e) {
        $report_data[$key]['status'] = FALSE;
      }

      // Confidence report data.
      try {
        $confidenceReport = $this->liftAPI->getConfidenceReport($machine_name, $date_from, $date_to, $decision_name, "all");
        $report_data[$key]['confidence'] = $this->extractConfidenceReportData($confidenceReport['data']['items']);
      }
      catch (AcquiaLiftException $e) {
        $report_data[$key]['confidence'] = FALSE;
      }

      // Targeting report data.
      try {
        $targetingReport = $this->liftAPI->getTargetingImpactReport($machine_name, $date_from, $date_to, $decision_name);
        $report_data[$key]['targeting'] = $this->extractTargetingReportData($targetingReport['data']['items']);
      }
      catch (AcquiaLiftException $e) {
        $report_data[$key]['targeting'] = FALSE;
      }
    }
    return $report_data[$key];
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

    // Determine the life of the agent as an interval.
    $interval_start = new DateTime();
    $interval_start->setTimestamp($this->agent->getStartTime());
    $interval = date_diff($interval_start, date_create());

    // Determine overall confidence based on confidence in choices.
    $confidence = $report_data['confidence'];
    $total_confident = 0;
    if (is_array($confidence)) {
      foreach($confidence['features'][self::NO_FEATURES] as $choice) {
        if ($choice['significant'] == TRUE) {
          $total_confident++;
        }
      }
    }

    // Create report renderable.
    $build = array();
    $build['test_type'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#title' => $report['decision_style'] === 'adaptive' ? t('Auto-personalize') : 'A/B',
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
      '#title' => isset($interval) ? $interval->format('%mm, %dd') : '1d',
      '#description' => t('total time running'),
    );
    $shown = $report_data['today_only'] ? $report['today_shown'] : $report['total_shown'];
    $build['shown'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-shown',
      ),
      '#title' => $this->formatReportNumber($shown),
      '#description' => format_plural($shown, 'time shown', 'times shown'),
    );
    $total_goals = $report_data['today_only'] ? $report['today_goals'] : $report['total_goals'];
    $build['goals'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-goals',
      ),
      '#title' => $this->formatReportNumber($total_goals),
      '#description' => t('goals met'),
    );
    if ($total_goals > 0) {
      $build['goals']['#attributes']['class'] = array('acquia-lift-report-positive');
    }
    $lift = $report_data['today_only'] ? $report['today_lift'] : $report['total_lift'];
    $build['lift'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-lift',
      ),
      '#title' => $this->formatReportPercentage($lift),
      '#description' => t('Predicted lift/control'),
      '#attributes' => array(
        'class' => array(
          $lift > self::LIFT_THRESHHOLD && $total_confident > 0 ? 'acquia-lift-report-positive' : 'acquia-lift-report-negative',
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
      '#description' =>  $total_confident > 0 ? format_plural($total_confident, 'High confidence, 1 var.', 'High confidence, @count vars.') : t('Low confidence'),
      '#attributes' => array(
        'class' => array(
          $total_confident > 0 ? 'acquia-lift-report-positive' : 'acquia-lift-report-negative',
        ),
      ),
    );
    return $build;
  }

  /**
   * Returns a render array for the content set variation experiment report.
   *
   * @param array $report_data
   *   An array of report data for the given dates and decision.
   * @return array
   *   A render array representing the variation set report.
   */
  protected function buildExperimentReport($report_data) {
    $build = array();
    if ($report_data['confidence'] === FALSE) {
      return $build;
    }

    // Determine possible goal value for entire campaign
    $goals = personalize_goal_load_by_conditions(array('agent' => $this->agent->getMachineName()));
    $total_goal_value = 0;
    foreach($goals as $goal) {
      $total_goal_value += $goal['value'];
    }

    $set_data = $report_data['confidence'];
    $fields = array(
      'counter' => t('Var.'),
      'choice_id' => t('Name'),
      'decisions' => t('Shown'),
      'estimated_value' => t('Predicted goal value'),
      'lift_default' => t('Lift over default'),
      'lift_random' => t('Lift over random'),
      'confidence' => t('Confidence'),
    );

    // Columns that should not be sortable.
    $exclude_sort = array('choice_id', 'confidence');

    // Not all of these columns will be shown, depending on certain factors.
    $exclude_columns = array();
    if ((isset($this->data['control_rate']) && (int) $this->data['control_rate'] === 0)) {
      $exclude_columns[] = 'lift_default';
    }

    // Generate the headers.
    foreach ($fields as $field_name => $display) {
      if (in_array($field_name, $exclude_columns)) {
        continue;
      }
      if (in_array($field_name, $exclude_sort)) {
        $header[] = $display;
      }
      else {
        // Handles displaying the sortable columns in standard Drupal theme.
        $header[] = array(
          'data' => $display,
          //'field' => $field_name,
        );
      }
    }

    // Generate the table data.
    $rows = array();
    foreach ($set_data['features'][self::NO_FEATURES] as $choice) {
      $row = array();
      foreach ($fields as $field_name => $display) {
        if (in_array($field_name, $exclude_columns)) {
          continue;
        }
        switch ($field_name) {
          case 'confidence':
            $row[] = array(
              'data' => $this->formatReportNumber($choice[$field_name]),
              'class' => $this->getGraphLevelClass($choice[$field_name]/100),
            );
            break;
          case 'decisions':
            $row[] = format_plural($choice[$field_name], '1 time', '@count times');
            break;
          case 'lift_default':
          case 'lift_random':
            $row[] = array(
              'data' => $this->formatReportNumber($choice[$field_name], FALSE),
              'class' => $choice[$field_name] > self::LIFT_THRESHHOLD ? array('acquia-lift-report-positive') : array('acquia-lift-report-negative'),
            );
            break;
          case 'goals_per_decision':
          case 'value_per_decision':
            $row[] = $this->formatReportNumber($choice[$field_name], FALSE);
            break;
          case 'estimated_value': {
            /*
            $row[] = theme('acquia_lift_high_low', array(
              'high' => $choice['estimated_lower'],
              'low' => $choice['estimated_lower'],
              'value' => $this->formatReportNumber($choice['estimated_value'], TRUE, 4),
            ));
            */
            $row[] = $this->formatReportNumber($choice['estimated_value'], TRUE, 4);
            break;
          }
          default:
            $row[] = $choice[$field_name];
        }
      }
      if ($choice['control'] === TRUE) {
        $rows[] = array(
          'data' => $row,
          'class' => array('acquia-lift-report-control'),
          'no_striping' => TRUE,
        );
      }
      else {
        $rows[] = $row;
      }
    }
    // Add the final row to display the total goal value
    $rows[] = array(
      'data' => array(
        '',
        '',
        '',
        array (
          'data' => t('Out of a possible @goal_value', array('@goal_value' => $total_goal_value)),
          'class' => array('acquia-lift-experiment-goal-total'),
        ),
        '',
        '',
        '',
      ),
      'class' => array('acquia-lift-experiment-footer-row'),
      'no_striping' => TRUE,
    );
    $build['content'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
    );
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

    if ($report_data['confidence'] === FALSE) {
      return array();
    }
    $header = array(
      array(
        'data' => t('Var.'),
        //'field' => 'counter',
      ),
      t('Name'),
      array(
        'data' => t('Context'),
        //'field' => 'feature',
        //'sort' => 'asc',
      ),
      array(
        'data' => t('Shown'),
        //'field' => 'decisions',
      ),
      array(
        'data' => t('Lift/control'),
        //'field' => 'lift_default',
      ),
      array(
        'data' => t('Lift/random'),
        //'field' => 'lift_random'
      )
    );
    $rows = array();
    foreach($report_data['confidence']['features'] as $feature_string => $feature) {
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

      // Don't show system-defined features.
      if ($targeting_data['system'] === TRUE) {
        continue;
      }
      foreach($feature as $choice) {
        $row = array();
        $row[] = $choice['counter'];
        $row[] = $choice['choice_id'];
        $row[] = array(
          'data' => $targeting_data['favored_selection'] === $choice['raw_label'] ? $feature_label . ' <span class="acquia-lift-best">' . t('best') . '</span>' : $feature_label,
          'class' => $targeting_data['favored_selection'] === $choice['raw_label'] ? array('acquia-lift-context-best') : array(),
        );
        $row[] = format_plural($choice['decisions'], '1 time', '@count times');
        if ($choice['control'] === TRUE) {
          $row[] = self::DATA_NA;
        }
        else {
          $row[] = array(
            'data' => $this->formatReportPercentage($choice['lift_default'], TRUE),
            'class' => $choice['lift_random'] > self::LIFT_THRESHHOLD ? array('acquia-lift-report-positive') : array('acquia-lift-report-negative'),
          );
        }
        $row[] = array(
          'data' => $this->formatReportPercentage($choice['lift_random'], TRUE),
          'class' => $choice['lift_random'] > self::LIFT_THRESHHOLD ? array('acquia-lift-report-positive') : array('acquia-lift-report-negative'),
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
    $header = array(
      array(
        'data' => t('Context'),
        //'field' => 'feature',
        //'sort' => 'asc',
      ),
      array(
        'data' => t('Best Variation'),
        //'field' => 'feature',
      ),
      array(
        'data' => t('Percent of Traffic'),
        //'field' => 'percent_traffic',
      ),
      t('Stability'),
    );
    $rows = array();
    foreach ($data as $feature => $f) {
      // Don't include system-defined features.
      if (!$f['system']) {
        // Get the user-friendly feature label from the possible contextual values.
        $feature_label = $feature;
        if (isset($report_data['potential_context'][$feature])) {
          $feature_label = $report_data['potential_context'][$feature]['label'];
        }

        $rows[] = array(
          'data' => array(
            $feature_label,
            $f['favored_selection_number'],
            array(
              'data' => $this->formatReportPercentage($f['percent_traffic']),
              'class' => array($this->getGraphLevelClass($f['percent_traffic'])),
            ),
            array(
              'data' => $this->formatReportNumber($f['stability']),
              'class' => $f['stability'] > self::STABILITY_THRESHOLD ? array('acquia-lift-report-positive') : array('acquia-lift-report-negative'),
            ),
          ),
          'data-acquia-lift-feature' => $feature,
        );
      }
    }
    $build['content'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
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
    $report['decision_style'] = $agent_data['decision_style'];
    $report['total_shown'] = $items['totals']['sessions']['count'];
    $report['total_goals'] = $items['totals']['goals']['count'];
    $report['today_shown'] = $items['today']['sessionCount'];
    $report['today_goals'] = $items['today']['goalCount'];
    $report['total_lift'] = $items['today']['liftOverDefaultUsingGoalsToDate'];
    $report['today_lift'] = $items['today']['liftOverDefaultUsingGoals'];
    $report['total_variations'] = count($agent_data['decisions']);
    return $report;
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
        if ($option_label = personalize_get_option_label_for_decision_and_choice($decision_name, $option_id)) {
          $choice_id = $option_label;
        }
      }
      $data['features'][$item['feature']][$choice] = array(
        'counter' => $counter,
        'choice_id' => $choice_id,
        'raw_label' => $option_id,
        'decisions' => $item['totals']['count'],
        'goals' => $item['totals']['goals'],
        'value' => $item['totals']['val'],
        'estimated_value' => $item['vMean'],
        'estimated_lower' => $item['bLo'],
        'estimated_higher' => $item['bHi'],
        'goals_per_decision' => $item['totals']['goals'] == 0 ? self::DATA_NA : $item['totals']['goalsPerDecision'],
        'value_per_decision' => $item['totals']['goals'] == 0 ? self::DATA_NA : $item['totals']['valPerDecision'],
        'selections' => $item['count'],
        'confidence' => $item['confidence'],
        'lift_default' => $item['lift']['default'],
        'lift_random' => $item['lift']['random'],
        'significant' => $item['signif'],
        'control' => $option_id === 'option-a' ? TRUE : FALSE,
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
        if ($choice['score'] > $item['choices'][$favored_selection]['score']) {
          $favored_selection = $i;
        }
      }
      $data[$feature] = array(
        'raw_label' => $item['label'],
        'label' => $item['labelText'],
        'favored_selection' =>  $item['choices'][$favored_selection]['label'],
        'favored_selection_number' => $favored_selection + 1,
        'percent_traffic' => $item['percentTraffic'],
        'predicted_value' => $item['averageResponseValue'],
        'stability' => $item['stability'],
        'stability_level' => $item['stabilityLevel'],
        'system' => strpos($item['label'], '[share-alt]') === FALSE ? FALSE : TRUE,
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
   * Formats a percentage value for use in reports.
   *
   * @param $value
   *   The number to show as a percentage.
   * @param bool $include_sign
   *   True to include positive/negative sign indicators.
   * @return string
   *   The formatted number to display.
   */
  protected function formatReportPercentage($value, $include_sign = FALSE) {
    $percent = (float) $value * 100;
    if ($percent > 0 && $include_sign) {
      return '+' . $this->formatReportNumber($percent) . '%';
    }
    return $this->formatReportNumber($percent) . '%';
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
   * @return string
   *   The formatted number to display.
   */
  protected function formatReportNumber($value, $trim = TRUE, $decimals = 2) {
    if (is_numeric($value)) {
      $value = number_format($value, $decimals);
      if ($trim) {
        $value = rtrim(trim($value, '0'), '.');
      }
      if (strlen($value) === 0) {
        $value = 0;
      }
      return $value;
    }
    return $value;
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
}
