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
   * Returns a render array representing the confidence report for the given dates/features.
   *
   * @param $date_start
   *   The start date of the report
   * @param $date_end
   *   The end date of the report
   * @param null $feature
   *   A feature to filter by or NULL to show data across all features.
   * @return array
   *   A render array representing the confidence report
   */
  public function buildConfidenceReport($date_start, $date_end, $decision_point, $feature = NULL);

  /**
   * Returns a render array representing the targeting report for the given dates.
   *
   * @param $date_start
   *   The start date of the report
   * @param $date_end
   *   The end date of the report
   * @return array
   *   A render array representing the targeting report
   */
  public function buildTargetingFeaturesReport($date_start, $date_end, $decision_point);

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

class AcquiaLiftAgent extends PersonalizeAgentBase implements AcquiaLiftAgentInterface, PersonalizeExplicitTargetingInterface {

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
      return new self($agent_data->machine_name, $agent_data->label, $agent_data->data, $agent_data->status, $acquia_lift_api);
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
  public function __construct($machine_name, $title, $data, $status, AcquiaLiftAPI $acquia_lift_api) {
    parent::__construct($machine_name, $title, $data, $status);
    $this->liftAPI = $acquia_lift_api;
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
            )
          )
        ),
        // Add the Acquia Lift API js wrapper and the Acquia Lift integration js.
        drupal_get_path('module', 'acquia_lift') . '/js/acquia-lift-js-wrapper-min.js' => array('type' => 'file', 'scope' => 'header'),
        drupal_get_path('module', 'acquia_lift') . '/js/acquia_lift.js' => array('type' => 'file', 'scope' => 'footer', 'defer' => TRUE),
      )
    );
  }

  public function useClientSideGoalDelivery() {
    return variable_get('acquia_lift_client_side_goals', TRUE);
  }

  /**
   * Implements PersonalizeAgentInterface::sendGoal().
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
      ),
      'js' => array(
        drupal_get_path('module', 'acquia_lift') . '/js/acquia_lift.agent.admin.js',
      ),
    );
    $option_parents = array('agent_basic_info', 'options', 'acquia_lift');
    $form['control'] = array(
      '#type' => 'fieldset',
      '#tree' => FALSE,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#attributes' => array('class' => array(
        'horizontal',
        'acquia-lift-more-information',
      )),
      '#title' => t('Control'),
    );
    $control_rate_parents = $option_parents;
    $control_rate_parents[] = 'control_rate';
    $form['control']['control_rate'] = array(
      '#type' => 'textfield',
      '#parents' => $control_rate_parents,
      '#title' => t('Control Group'),
      '#field_suffix' => '%',
      '#size' => 3,
      '#description' => t('A fixed baseline variation will be shown, by default the first variation in the set.'),
      '#default_value' => isset($agent_data->data['control_rate']) ? $agent_data->data['control_rate'] : 10,
      '#attributes' => array(
        'class' => array('acquia-lift-agent-percentage-adjust'),
        'data-acquia-lift-percentage-rest' => 'acquia-lift-control-rate-rest',
      ),
    );
    $form['control']['group_rate'] = array(
      '#type' => 'textfield',
      '#title' => t('Test Group'),
      '#field_suffix' => '%',
      '#size' => 3,
      '#description' => t('Personalized variations will be shown.'),
      '#default_value' => isset($agent_data->data['control_rate']) ? (100 - $agent_data->data['control_rate']) : 90,
      '#disabled' => TRUE,
      '#attributes' => array(
        'id' => 'acquia-lift-control-rate-rest',
        'class' => array('acquia-lift-percentage-rest'),
      ),
    );
    $form['decision_style'] = array(
      '#type' => 'radios',
      '#title' => t('Decision Style'),
      '#options' => array('adaptive' => t('Auto-personalize'), 'random' => t('Test only')),
      '#default_value' => isset($agent_data->data['decision_style']) ? $agent_data->data['decision_style'] : 'adaptive',
      '#description' => t('Auto personalize chooses the best option over time, test just shows variations and reports results'),
      '#title_display' => 'invisible',
    );
    $form['distribution'] = array(
      '#type' => 'fieldset',
      '#tree' => FALSE,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#attributes' => array('class' => array(
        'horizontal',
        'acquia-lift-more-information',
      )),
      '#title' => t('Distribution'),
      '#states' => array(
        'visible' => array(
          ':input[name="agent_basic_info[options][acquia_lift][decision_style]"]' => array('value' => 'adaptive'),
        ),
      ),
    );
    $explore_rate_parents = $option_parents;
    $explore_rate_parents[] = 'explore_rate';
    $form['distribution']['explore_rate'] = array(
      '#type' => 'textfield',
      '#parents' => $explore_rate_parents,
      '#title' => t('Random Group'),
      '#field_suffix' => '%',
      '#description' => t('Variations will be shown randomly and tracked to adjust for false positives.'),
      '#size' => 3,
      '#default_value' => isset($agent_data->data['explore_rate']) ? $agent_data->data['explore_rate'] : 20,
      '#attributes' => array(
        'class' => array('acquia-lift-agent-percentage-adjust'),
        'data-acquia-lift-percentage-rest' => 'acquia-lift-explore-rate-rest',
      ),
    );
    $form['distribution']['exploit_rate'] = array(
      '#type' => 'textfield',
      '#title' => t('Personalized Group'),
      '#field_suffix' => '%',
      '#size' => 3,
      '#description' => t('The "best" variation will be shown for each visitor based on our algorithm.'),
      '#default_value' => isset($agent_data->data['explore_rate']) ? (100 - $agent_data->data['explore_rate']) : 80,
      '#disabled' => TRUE,
      '#attributes' => array(
        'id' => 'acquia-lift-explore-rate-rest',
        'class' => array('acquia-lift-percentage-rest'),
      )
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
    return PersonalizeExplicitTargetingInterface::EXPLICIT_TARGETING_MULTIPLE_OR;
  }

  /**
   * Implements PersonalizeAgentInterface::convertContextToFeatureString().
   */
  public static function convertContextToFeatureString($name, $value) {
    // This is essentially a PHP version of the js in the convertContextToFeatureString
    // function acquia_lift.js
    $prefix = AcquiaLiftAPI::cleanFeatureString($name);
    $value = AcquiaLiftAPI::cleanFeatureString($value);
    $feature_string = $prefix . '--' . $value;
    // Make a string of the visitor context item in the form name--value
    while (strlen($feature_string) > AcquiaLiftAPI::FEATURE_STRING_MAX_LENGTH) {
      // AcquiaLift has a hard limit of 25 characters for feature strings so
      // if our name--value string is too long, start by whittling down the
      // length of the name part and remove it if necessary.
      if (strlen($prefix) > 1) {
        $prefix = substr($prefix, 0, strlen($prefix));
        $feature_string = $prefix . '--' . $value;
      }
      else {
        $feature_string = substr($value, 0, AcquiaLiftAPI::FEATURE_STRING_MAX_LENGTH);
      }
    }
    return $feature_string;
  }

  /**
   * Implements AcquiaLiftAgent::buildConfidenceReport().
   */
  public function buildConfidenceReport($date_start, $date_end, $decision_point, $feature = NULL) {
    $build = array();
    $confidenceReport = $this->liftAPI->getConfidenceReport($this->machineName, $date_start, $date_end, $decision_point, array($feature));
    $data = $this->extractConfidenceReportData($confidenceReport['data']['items']);

    $column_headings = array(
      'choice_id' => t('Variation'),
      'decisions' => t('Decisions'),
      'goals' => t('Goals'),
      'value' => t('Value'),
      'goals_per_decision' => t('Goals / visitor'),
      'value_per_decision' => t('Value / visitor'),
      'estimated_value' => t('Estimated Value'),
      'lift_default' => t('Lift over default'),
      'lift_random' => t('Lift over random'),
      'confidence' => t('Confidence')
    );
    // Not all of these columns will be shown, depending on certain factors.
    $exclude_columns = array();
    if (isset($this->data['control_rate']) && (int) $this->data['control_rate'] == 0) {
      $exclude_columns[] = 'lift_default';
    }
    if (!$data['goal_value_differential']) {
      $exclude_columns = array_merge($exclude_columns, array(
        'value',
        'value_per_decision',
      ));
    }

    // Build the header and rows.
    $header = $rows = array();
    foreach ($column_headings as $key => $heading) {
      if (in_array($key, $exclude_columns)) {
        continue;
      }
      $header[] = $heading;
    }
    foreach ($data['choices'] as $choice) {
      $row = array();
      foreach ($column_headings as $key => $heading) {
        if (in_array($key, $exclude_columns)) {
          continue;
        }
        $row[] = $choice[$key];
      }
      $rows[] = $row;
    }

    $build['content'] = array(
      'content' =>  array(
        '#theme' => 'table',
        '#header' => $header,
        '#rows' => $rows,
      )
    );
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
  protected function extractConfidenceReportData($items) {
    if (empty($items)) {
      return array();
    }
    $data = array(
      'point' => $items[0]['point'],
      'choices' => array(),
      'goal_value_differential' => FALSE,
    );
    foreach ($items as $i => $item) {
      $choice = $item['choice'];
      if (isset($data['choices'][$choice])) {
        // The totals without segment/feature breakout come first and are the
        // only piece we're interested in for this report.
        continue;
      }
      $choice_id = $choice;
      if (strpos($choice, ':') !== FALSE) {
        list($decision_name, $option_id) = explode(':', $choice);
        if ($option_label = personalize_get_option_label_for_decision_and_choice($decision_name, $option_id)) {
          $choice_id = $option_label;
        }
      }
      $data['choices'][$choice] = array(
        'choice_id' => $choice_id,
        'decisions' => $item['totals']['count'],
        'goals' => $item['totals']['goals'],
        'value' => $item['totals']['val'],
        'estimated_value' => $item['vMean'],
        'goals_per_decision' => $item['totals']['goalsPerDecision'],
        'value_per_decision' => $item['totals']['valPerDecision'],
        'selections' => $item['count'],
        'confidence' => $item['confidence'] . '%',
        'lift_default' => $i == 0 ? '--' : $item['lift']['default'] . '%',
        'lift_random' => $item['lift']['random'] . '%',
      );
      if (!$data['goal_value_differential'] && ($item['totals']['goals'] != $item['totals']['val'])) {
        $data['goal_value_differential'] = TRUE;
      }
    }
    return $data;
  }

  /**
   * Implements AcquiaLiftAgent::buildTargetingFeaturesReport().
   */
  public function buildTargetingFeaturesReport($date_start, $date_end, $decision_point) {
    $build = array();
    $report = $this->liftAPI->getTargetingImpactReport($this->machineName, $date_start, $date_end, $decision_point);
    $data = $this->extractTargetingReportData($report['data']['items']);
    $header = array(
      t('Context'), t('Best Variation'), t('Percent of Traffic'), t('Predicted Value'), t('Stability')
    );
    $rows = array();
    foreach ($data as $feature => $f) {
      if (strpos($f['label'], '[share-alt]') === FALSE) {
        $rows[] = array(
          $feature,
          $f['favored_selection'],
          round($f['percent_traffic'] * 100, 2) . '%',
          round($f['predicted_value'], 2),
          round($f['stability'], 2)
        );
      }

    }
    $build['content'] = array(
      'content' =>  array(
        '#theme' => 'table',
        '#header' => $header,
        '#rows' => $rows,
      )
    );
    return $build;
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
        'label' => $item['label'],
        'favored_selection' =>  $item['choices'][$favored_selection]['label'],
        'percent_traffic' => $item['percentTraffic'],
        'predicted_value' => $item['averageResponseValue'],
        'stability' => $item['stability'],
      );
    }
    return $data;
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
   * Implements PersonalizeAgentInterface::verify().
   */
  public function verify() {
    $acquia_lift_agent = $this->liftAPI->getAgent($this->machineName);
    if (!$acquia_lift_agent || $acquia_lift_agent['status'] !== 'enabled') {
      return FALSE;
    }
    // Make sure Acquia Lift knows about the agent's goals.
    $goals = personalize_goal_load_by_conditions(array('agent' => $this->machineName));
    $acquia_lift_goals = $this->liftAPI->getGoalsForAgent($this->machineName);
    if (empty($goals) || empty($acquia_lift_goals)) {
      // Acquia Lift agents need goals.
      return FALSE;
    }

    foreach($goals as $goal) {
      if (!in_array($goal['action'], $acquia_lift_goals)) {
        return FALSE;
      }
    }
    // Make sure all decision points are known by Acquia Lift.
    $option_sets = personalize_option_set_load_by_agent($this->machineName);
    $decision_tree = self::convertOptionSetsToDecisions($option_sets);
    $acquia_lift_points = $this->liftAPI->getPointsForAgent($this->machineName);
    foreach ($decision_tree as $point => $decisions) {
      if (!in_array($point, $acquia_lift_points)) {
        return FALSE;
      }
      $acquia_lift_decisions = $this->liftAPI->getDecisionsForPoint($this->machineName, $point);
      foreach ($decisions as $decision_name => $options) {
        if (!in_array($decision_name, $acquia_lift_decisions)) {
          return FALSE;
        }
        $acquia_lift_choices = $this->liftAPI->getChoicesForDecision($this->machineName, $point, $decision_name);
        foreach ($options as $option) {
          if (!in_array($option, $acquia_lift_choices)) {
            return FALSE;
          }
        }
      }
      // @todo Check the fixed targeting for each decision point.
    }
    return TRUE;
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
          foreach ($option['fixed_targeting'] as $feature) {
            $mappings[$point_name][] = array(
              'feature' => $feature,
              'decision' => $decision_name . ':' . $option['option_id']
            );
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
