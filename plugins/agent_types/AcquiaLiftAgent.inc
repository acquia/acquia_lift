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
              'batchMode' => FALSE
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
