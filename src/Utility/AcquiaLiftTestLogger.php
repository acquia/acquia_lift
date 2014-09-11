<?php

namespace Drupal\acquia_lift\Utility;

/**
 * Class AcquiaLiftTestLogger
 */
class AcquiaLiftTestLogger implements PersonalizeLoggerInterface {
  protected $logs = array();

  public function log($level, $message, array $context = array())
  {
    $this->logs[] = array(
    'level' => $level,
    'message' => $message,
    );
  }

  public function clearLogs() {
    $this->logs = array();
  }

  public function getLogs() {
    return $this->logs;
  }
}
