<?php

interface AcquiaLiftAPIInterface {
  public function getAgent($agent_name);

  /**
   * Returns the list of agent names that already exist in Lift.
   * @return mixed
   *   An array of agent names.
   */
  public function getExistingAgentNames();
  public function getGoalsForAgent($agent_name);
  public function deleteAgent($agent_name);
  public function ping();
  public function ensureUniqueAgentName($agent_name, $max_length);
}
