<?php

class AcquiaLiftAPIClientFactory {

  /**
  * @param $account_info
  * @param string $version
  * @return AcquiaLiftAPIInterface
  */
  public static function create($account_info, $version = ACQUIA_LIFT_TESTING_AGENT_V1) {
    $class_map = array(
      ACQUIA_LIFT_TESTING_AGENT_V1 => 'AcquiaLiftAPI',
      ACQUIA_LIFT_TESTING_AGENT_V2 => 'AcquiaLiftAPIV2',
    );
    $class = $class_map[$version];
    return $class::getInstance($account_info);
  }
}
