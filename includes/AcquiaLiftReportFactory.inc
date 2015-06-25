<?php

/**
 * Factory class to create a report object for Acquia Lift.
 */
class AcquiaLiftReportFactory {

  /**
   * Creates a Report object.
   *
   * @param $agent_instance
   *   The Acquia Lift agent that the reporting is for.
   * @param $api_instance
   *   The Acquia Lift API classes in use.
   * @return
   *   The AcquiaLiftReport class to use.
   */
  public static function create(AcquiaLiftLearningAgentInterface $agent_instance, $api_instance, $options) {
    // Load all reporting-related classes.
    $report_class_files = array(
      'AcquiaLiftReportBase',
      'AcquiaLiftReport',
      'AcquiaLiftReportCacheInterface',
      'AcquiaLiftReportCache',
      'AcquiaLiftReportDataFromFile',
    );
    foreach ($report_class_files as $file) {
      module_load_include('inc', 'acquia_lift', 'includes/' . $file);
    }
    $agent_type = $agent_instance->getType();
    switch ($agent_type) {
      case ACQUIA_LIFT_TESTING_AGENT_V2:
        $class = 'AcquiaLiftLearnReport';
        $file_source_class = 'AcquiaLiftLearnReportFromFile';
        break;
      case ACQUIA_LIFT_TESTING_AGENT_V1;
      default:
        $class = 'AcquiaLiftReport';
        $file_source_class = 'AcquiaLiftReportDataFromFile';
        if (!empty($options['legacy'])) {
          $class = 'AcquiaLiftReportLegacy';
        }
        break;
    }
    if (!isset($options['confidence_measure'])) {
      $options['confidence_measure'] = variable_get('acquia_lift_confidence_measure', 95);
    }
    $agent_name = $agent_instance->getMachineName();
    // Check if this agent is set up to read reports from a file instead of
    // calling the API.
    $report_file = variable_get("acquia_lift_report_source_$agent_name", '');
    if (!empty($report_file)) {
      $report_source = new $file_source_class($report_file, new AcquiaLiftReportCache());
    }
    else {
      $report_source = $api_instance;
    }
    $report = new $class($agent_instance, $options, $report_source);
    return $report;
  }
}