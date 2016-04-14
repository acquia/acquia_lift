// set AcuiaLift settings from Drupal settings
if(drupalSettings) {
	if(drupalSettings.acquia_lift) {
		var accountId = drupalSettings.acquia_lift.credential.account_name;
		var siteId = drupalSettings.acquia_lift.credential.customer_site;
		window.AcquiaLift = { "account_id" : accountId, "site_id" : siteId };
	}
}
