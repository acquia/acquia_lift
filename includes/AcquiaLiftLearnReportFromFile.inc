<?php

/**
 * Class for Acquia Lift Learn Reports read from a file.
 */

class AcquiaLiftLearnReportFromFile extends AcquiaLiftReportDataFromFileBase implements AcquiaLiftLearnReportDataSourceInterface {
  public function getReportForDateRange($name, $from, $to) {
    return $this->getReport($name);
  }
}