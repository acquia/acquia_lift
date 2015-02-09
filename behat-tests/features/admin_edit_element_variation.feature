#Assumes Marketer role.
Feature: Personalize elements variations can be edited for an existing campaign.
  In order to manage element variations in context
  As a site marketer
  I want the ability to edit existing personalize element variations in context.

  @api @javascript @campaign
  Scenario: Edit existing personalize elements for an acquia_lift campaign.
    Given "acquia_lift" agents:
      | machine_name                    | label |
      | testing-campaign-edit-variations | Testing campaign edit variations |
    And personalized elements:
      | label                     | agent                            | selector     | type     | content                 |
      | Site name updated         | testing-campaign-edit-variations | #site-name a | editText | The Rainbow Connection  |
    And I am logged in as a user with the "Marketer" role
    And I am on the homepage
    When I click "Acquia Lift" in the "menu" region
    And I wait for AJAX to finish
    Then I should see the link "Campaigns" in the "lift_tray" region
    When I hover over "Campaigns" in the "lift_tray" region
    And I click "Testing campaign edit variations" in the "lift_tray" region
    And I wait for AJAX to finish
    Then I should see the link "Variation Sets" visible in the "lift_tray" region
    And I should see the text "1" in the "lift_tray_variation_count" region
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should see the text "Site name updated" in the "lift_tray" region
    And I should see the link "Option A" visible in the "lift_tray" region
    When I click "Edit" link for the "Site name updated" set "Option A" variation
    And I wait for AJAX to finish
    Then I should see the text "Edit text: <A>" in the "dialog_variation_type_form" region
    And the "personalize_elements_content" field should contain "The Rainbow Connection"
    And the "option_label" field should contain "Option A"
    When I fill in "Variation 1" for "option_label"
    And I fill in "Moving Right Along" for "personalize_elements_content"
    And I press the "Save" button
    And I wait for AJAX to finish
    Then I should see the message "The variation has been updated." in the messagebox
    When I wait for AJAX to finish
    Then I should not see the variation type form dialog
    And I should see the text "Moving Right Along" in the "page_content" region
    And I should see the text "1" in the "lift_tray_variation_count" region
    When I hover over "Variation Sets" in the "lift_tray" region
    And I wait for AJAX to finish
    Then I should see the text "Site name updated" in the "lift_tray" region
    And I should see the link "Variation 1" visible in the "lift_tray" region
