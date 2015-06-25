<?php

/**
 * Class AcquiaLiftReportCache
 *
 * An OOP wrapper around cache_get() and cache_set().
 */
class AcquiaLiftReportCache implements AcquiaLiftReportCacheInterface {
  /**
   * Holds the reports retrieved from the cache.
   *
   * @var array
   */
  protected $cache = array();

  /**
   * The cache bin to use.
   *
   * @var string
   */
  protected $bin = 'cache_acquia_lift_reports';

  /**
   * Implements AcquiaLiftReportCacheInterface::getCachedReports().
   */
  public function getCachedReports($agent_name) {
    if (!isset($this->cache[$agent_name])) {
      if ($get = cache_get($agent_name, $this->bin)) {
        $this->cache[$agent_name] = $get->data;
      }
      else {
        $this->cache[$agent_name] = FALSE;
      }
    }
    return $this->cache[$agent_name];
  }

  /**
   * Implements AcquiaLiftReportCacheInterface::cacheReports().
   */
  public function cacheReports($agent_name, $data) {
    cache_set($agent_name, $data, $this->bin);
  }
}
