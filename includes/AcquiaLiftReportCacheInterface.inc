<?php

interface AcquiaLiftReportCacheInterface {
  /**
   * Retrieve reports from the cache for a particular agent.
   *
   * @param $agent_name
   *   The name of the agent to retrieve reports for.
   * @return mixed
   *   An array representing the reports or NULL of not found.
   */
  public function getCachedReports($agent_name);

  /**
   * Store reports for a particular agent in the cache.
   *
   * @param $agent_name
   *   The name of the agent to store reports for.
   * @param $data
   *   An array representing the reports.
   */
  public function cacheReports($agent_name, $data);
}
