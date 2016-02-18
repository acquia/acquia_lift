<?php
/**
 * @file
 * Provides a learning agent class for Acquia Lift
 */

class AcquiaLiftLearn extends PersonalizeAgentBase implements AcquiaLiftLearningAgentInterface, PersonalizeAgentGoalInterface {

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
      $acquia_lift_api = self::getAPIInstance();
      $status = personalize_agent_get_status($agent_data->machine_name);
      return new static($agent_data->machine_name, $agent_data->label, $agent_data->data, $status, !empty($agent_data->started) ? $agent_data->started : NULL, $acquia_lift_api);
    }
    catch (AcquiaLiftException $e) {
      watchdog('Acquia Lift', 'Unable to instantiate Acquia Lift Learning Agent');
      return NULL;
    }
  }

  /**
   * Implements AcquiaLiftLearningAgentInterface::getAPIInstance().
   */
  public static function getAPIInstance() {
    $account_info = acquia_lift_get_account_info();
    $acquia_lift_api = AcquiaLiftAPI::getInstance($account_info);
    return $acquia_lift_api;
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
    parent::__construct($machine_name, $title, $data, $status, $started);
    $this->liftAPI = $acquia_lift_api;
  }

  /**
   * Implements PersonalizeAgentInterface::getType();
   */
  public function getType() {
    return ACQUIA_LIFT_TESTING_AGENT;
  }

  /**
   * Implements AcquiaLiftLearningAgentInterface::getAgentSyncOperations().
   */
  public function getAgentSyncOperations($option_sets, $new_goals, $old_goals = array()) {
    // Currently only a single option set is supported.
    $option_set = reset($option_sets);
    $max_length = AcquiaLiftAPI::NAME_MAX_LENGTH;
    // If the test agent has a site name, use this as a prefix for goals and
    // decision sets.
    $prefix = empty($this->data['site_name']) ?  '' : $this->data['site_name'] . '-';
    $decision_name = $option_set->decision_name;
    while (strlen($prefix . $decision_name) > $max_length) {
      $decision_name = substr($decision_name, 0, -1);
    }
    // Decision set needs to be saved first.
    $items[] = array(
      'method' => 'saveDecisionSet',
      'args' => array(
        $prefix . $decision_name,
        $this->title,
        $option_set->options
      ),
    );
    $goal_names = array();

    foreach ($new_goals as $goal_name => $goal_value) {
      while (strlen($prefix . $goal_name) > $max_length) {
        $goal_name = substr($goal_name, 0, -1);
      }
      $goal_names[] = $prefix . $goal_name;
      $items[] = array(
        'method' => 'saveGoal',
        'args' => array(
          $prefix . $goal_name
        ),
      );
    }
    $acquia_lift_control_rate = 0;
    $acquia_lift_explore_rate = 1;
    if (isset($this->data['control_rate'])) {
      // Acquia Lift takes the control rate as a number between 0 and 1.
      $acquia_lift_control_rate = $this->data['control_rate'] / 100;
    }
    if (isset($this->data['decision_style']) && $this->data['decision_style'] === 'adaptive') {
      // Default the explore rate to .2 for adaptive agents.
      $acquia_lift_explore_rate = .2;
      if (isset($this->data['explore_rate'])) {
        // Acquia Lift takes the explore rate as a number between 0 and 1.
        $acquia_lift_explore_rate = $this->data['explore_rate'] / 100;
      }
    }
    // Add an item for saving the agent to Acquia Lift.
    $items[] = array(
      'method' => 'saveCampaign',
      'args' => array(
        $this->machineName,
        $this->title,
        $prefix . $decision_name,
        $goal_names,
        $this->data['decision_style'] == 'adaptive',
        $acquia_lift_control_rate,
        $acquia_lift_explore_rate
      ),
    );

    return $items;
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
   * Whether to use the client-side goal delivery mechanism.
   *
   * @return boolean
   */
  public function useClientSideGoalDelivery() {
    return TRUE;
  }

  /**
   * Sends a goal to an agent.
   *
   * Only for server-side goal sending.
   *
   * @param string $goal_name
   *   The name of the goal being sent.
   * @param int $value
   *   (optional) The value of the goal being sent.
   */
  public function sendGoal($goal_name, $value = NULL) {

  }

}
