@api @campaign @javascript @insulated @targeting
Feature:  Campaign management for Acquia Lift Target campaigns
  In order to create and manage targeting campaigns
  As a site marketer
  I want a user interface that simplifies the underlying campaign structure.

  Scenario: Assign variations to audiences using drag and drop interface.
    Given "acquia_lift_target" agents:
      | machine_name                    | label                |
      | test-targeting-agent            | Test targeting agent |
    And personalized elements:
      | label                     | agent                | selector       | type     | content                       |
      | Variation set #1          | test-targeting-agent | #some-selector | editText | Option A, Option B            |
    And audiences:
      | label              | agent                | weight   |
      | Test audience      | test-targeting-agent | 20       |
      | Everyone else      | test-targeting-agent | 30       |
    And I am logged in as a user with the "access administration pages,access toolbar,administer visitor actions,manage personalized content" permission
    And I am on "admin/structure/personalize/manage/test-targeting-agent/targeting"

    # All variations should be selected in the "Everyone else" audience by default.
    Then "Option A" variation should be assigned to the "Everyone else" audience
    And "Option B" variation should be assigned to the "Everyone else" audience
    And "Option A" variation should not be assigned to the "Test audience" audience
    And "Option B" variation should not be assigned to the "Test audience" audience

    # Test simple assignment of a single variation.
    When I move the "Option A" variation from the "Everyone else" audience to the "Test audience" audience
    Then "Option A" variation should be assigned to the "Test audience" audience
    And "Option A" variation should not be assigned to the "Everyone else" audience

    # Test dropping a variation on its own audience's drop area.
    When I move the "Option A" variation from the "Test audience" audience to the "Test audience" audience
    Then "Option A" variation should be assigned to the "Test audience" audience
    And "Option A" variation should not be assigned to the "Everyone else" audience

    # Test duplication of a variation.
    When I copy the "Option B" variation from the "Everyone else" audience to the "Test audience" audience
    Then "Option B" variation should be assigned to the "Test audience" audience
    And "Option A" variation should be assigned to the "Test audience" audience
    And "Option B" variation should be assigned to the "Everyone else" audience

    # Test removal of a variation.
    When I move the "Option A" variation from the "Test audience" audience to the "Everyone else" audience
    Then "Option A" variation should be assigned to the "Everyone else" audience
    And "Option A" variation should not be assigned to the "Test audience" audience
    And "Option B" variation should be assigned to the "Test audience" audience
