#Assumes Marketer role.
Feature: Personalize elements variations can be edited for an existing campaign.
  In order to manage element variations in context
  As a site marketer
  I want the ability to add, edit, and delete existing personalize element variations in context.

  @api @javascript @campaign
  Scenario: Add personalize elements to a campaign
    Given "acquia_lift" agents:
      | machine_name                    | label |
      | testing-campaign-add-variations | Testing campaign add variations |
    And I am logged in as a user with the "Marketer" role
    And I am on the homepage
    When I click "Acquia Lift" in the "menu" region
    And I wait for AJAX to finish
    Then I should see the link "Campaigns" in the "lift_tray" region
    When I hover over "Campaigns" in the "lift_tray" region
    And I click "Testing campaign add variations" in the "lift_tray" region
    Then I should see the link "Variation Sets" visible in the "lift_tray" region
    And I should see "0" for the "variation set" count
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should see the link "Add variation set" in the "lift_tray" region
    When I click "Add variation set"
    And I wait for AJAX to finish
    Then I should see the modal with title "Add a variation set"
    And I should see the link "Webpage elements" in the "modal_content" region
    And I should see the link "Drupal blocks" in the "modal_content" region
    When I click "Webpage elements" in the "modal_content" region
    And I wait for AJAX to finish
    Then I should not see the modal
    When I click "#site-name a span" element in the "page_content" region
    And I wait for AJAX to finish
    Then I should see the text "<SPAN>" in the "dialog_variation_type" region
    When I click "Edit text" in the "dialog_variation_type" region
    And I wait for AJAX to finish
    Then I should not see the variation type dialog
    And I should see the text "Edit text: <SPAN>" in the "dialog_variation_type_form" region
    And the "personalize_elements_content" field should contain the site title
    When I fill in "Test 1" for "personalize_elements_content"
    And I fill in "Test variation set" for "title"
    And I press the "Save" button
    And I wait for AJAX to finish
    Then I should see the message "The variation set has been created." in the messagebox
    When I wait for AJAX to finish
    Then I should not see the variation type form dialog
    And I should see the text "Test 1" in the "page_content" region
    And I should see "1" for the "variation set" count
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should see the text "Test variation set" in the "lift_tray" region
    And I should see the link "Control variation" in the "lift_tray" region
    And I should see the link "Variation #1" in the "lift_tray" region
    And I should see the link "Add variation" in the "lift_tray" region
    And I should see the link "Add variation set" in the "lift_tray" region
    When I click "Add variation" in the "lift_tray" region
    Then "#site-name a span" element in the "page_content" region should have "acquia-lift-page-variation-item" class
    And I should see the text "Edit text: <SPAN>" in the "dialog_variation_type_form" region
    And I should not see the link "Edit selector" visible in the "dialog_variation_type_form" region
    When I fill in "Test 2" for "personalize_elements_content"
    And I press the "Save" button
    And I wait for AJAX to finish
    Then I should see the message "The variation has been created." in the messagebox
    When I wait for AJAX to finish
    Then I should not see the variation type form dialog
    And I should see the text "Test 2" in the "page_content" region
    And I should see "1" for the "variation set" count
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should see the link "Variation #2" in the "lift_tray" region

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
    And I should see "1" for the "variation set" count
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should see the text "Site name updated" in the "lift_tray" region
    And I should see the link "Option A" visible in the "lift_tray" region
    And "Site name updated" set "Control variation" variation should not have the "Edit" link
    And "Site name updated" set "Control variation" variation should not have the "Delete" link
    And "Site name updated" set "Option A" variation should have the "Edit" link
    And "Site name updated" set "Option A" variation should have the "Delete" link
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
    And I should see "1" for the "variation set" count
    When I hover over "Variation Sets" in the "lift_tray" region
    And I wait for AJAX to finish
    Then I should see the text "Site name updated" in the "lift_tray" region
    And I should see the link "Variation 1" visible in the "lift_tray" region

  @api @javascript @campaign
  Scenario: Delete an existing personalize element variation for an acquia_lift campaign.
    Given "acquia_lift" agents:
      | machine_name                    | label |
      | testing-campaign-edit-variations | Testing campaign edit variations |
    And personalized elements:
      | label                     | agent                            | selector     | type     | content                 |
      | Site name updated         | testing-campaign-edit-variations | #site-name a | editText | The Rainbow Connection, Moving Right Along  |
    And I am logged in as a user with the "Marketer" role
    And I am on the homepage
    When I click "Acquia Lift" in the "menu" region
    And I wait for AJAX to finish
    Then I should see the link "Campaigns" in the "lift_tray" region
    When I hover over "Campaigns" in the "lift_tray" region
    And I click "Testing campaign edit variations" in the "lift_tray" region
    And I wait for AJAX to finish
    Then I should see the link "Variation Sets" visible in the "lift_tray" region
    And I should see "1" for the "variation set" count
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should see the text "Site name updated" in the "lift_tray" region
    And I should see the link "Option A" visible in the "lift_tray" region
    And I should see the link "Option B" visible in the "lift_tray" region
    And "Site name updated" set "Control variation" variation should not have the "Edit" link
    And "Site name updated" set "Control variation" variation should not have the "Delete" link
    And "Site name updated" set "Option A" variation should have the "Edit" link
    And "Site name updated" set "Option A" variation should have the "Delete" link
    And "Site name updated" set "Option B" variation should have the "Edit" link
    And "Site name updated" set "Option B" variation should have the "Delete" link
    When I click "Delete" link for the "Site name updated" set "Option A" variation
    And I wait for AJAX to finish
    Then I should see the modal with title "Delete variation"
    When I press "Delete"
    And I wait for AJAX to finish
    Then I should see the message "The variation has been deleted" in the messagebox
    When I wait for AJAX to finish
    Then I should see "1" for the "variation set" count
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should see the text "Site name updated" in the "lift_tray" region
    And I should not see the link "Option A" visible in the "lift_tray" region
    And I should see the link "Option B" visible in the "lift_tray" region
    And "Site name updated" set "Control variation" variation should not have the "Edit" link
    And "Site name updated" set "Control variation" variation should not have the "Delete" link
    And "Site name updated" set "Option B" variation should have the "Edit" link
    And "Site name updated" set "Option B" variation should have the "Delete" link
    When I click "Delete" link for the "Site name updated" set "Option B" variation
    And I wait for AJAX to finish
    Then I should see the modal with title "Delete variation"
    When I press "Delete"
    And I wait for AJAX to finish
    Then I should see the message "The variation set has been deleted" in the messagebox
    When I wait for AJAX to finish
    Then I should see "0" for the "variation set" count
    When I hover over "Variation Sets" in the "lift_tray" region
    Then I should not see the text "Site name updated" in the "lift_tray" region
    And I should not see the link "Option B" visible in the "lift_tray" region
    And I should not see the link "Control variation" visible in the "lift_tray" region
    And I should see the text "No variations" in the "lift_tray" region
