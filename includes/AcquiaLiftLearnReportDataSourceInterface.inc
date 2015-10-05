<?php

/**
 * Interface for report data source class that Lift Learn Reports can use.
 */
interface AcquiaLiftLearnReportDataSourceInterface {
  public function getReportForDateRange($name, $from, $to);

  //public function getPersonalizationOverviewReport($name, $variations);

  //public function getAudienceReport($name, $audience, $variations, $type, $from, $to);
}
