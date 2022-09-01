<?php

namespace Drupal\acquia_lift_publisher\EventSubscriber\Publish;

use Acquia\ContentHubClient\CDF\CDFObject;
use Drupal\acquia_contenthub\AcquiaContentHubEvents;
use Drupal\acquia_contenthub\ContentHubCommonActions;
use Drupal\acquia_contenthub\Event\PrunePublishCdfEntitiesEvent;
use Drupal\acquia_contenthub_publisher\ContentHubPublisherEvents;
use Drupal\acquia_contenthub_publisher\Event\ContentHubEntityEligibilityEvent;
use Drupal\acquia_lift_publisher\Form\ContentPublishingForm;
use Drupal\acquia_lift_publisher\Form\ContentPublishingSettingsTrait;
use Drupal\Core\Config\ImmutableConfig;
use Symfony\Component\EventDispatcher\EventSubscriberInterface;

/**
 * Responsible for filtering contents based on publishing configurations.
 *
 * @see \Drupal\acquia_lift_publisher\Form\ContentPublishingForm::$pushSettingField
 * @package Drupal\acquia_lift_publisher\EventSubscriber\Publish
 */
class PublishOnlyRendered implements EventSubscriberInterface {

  use ContentPublishingSettingsTrait;

  /**
   * Content Hub common actions.
   *
   * @var \Drupal\acquia_contenthub\ContentHubCommonActions
   */
  private $commonActions;

  /**
   * PublishOnlyRendered constructor.
   *
   * @param \Drupal\acquia_contenthub\ContentHubCommonActions $common_actions
   *   The Content Hub common actions service.
   * @param \Drupal\Core\Config\ImmutableConfig $config
   *   The publisher configuration object.
   */
  public function __construct(ContentHubCommonActions $common_actions, ImmutableConfig $config) {
    $this->commonActions = $common_actions;
    $this->publisherSettings = $config;
  }

  /**
   * {@inheritdoc}
   */
  public static function getSubscribedEvents() {
    return [
      ContentHubPublisherEvents::ENQUEUE_CANDIDATE_ENTITY => ['onEnqueueCandidateEntity', 99],
      AcquiaContentHubEvents::PRUNE_PUBLISH_CDF_ENTITIES => ['onPrunePublishCdfEntities', 1000],
    ];
  }

  /**
   * Checks if the entity in hand is a renderable entity.
   *
   * Only enqueue the entity if it is renderable.
   *
   * @param \Drupal\acquia_contenthub_publisher\Event\ContentHubEntityEligibilityEvent $event
   *   The current enqueue eligibility event.
   *
   * @throws \Exception
   */
  public function onEnqueueCandidateEntity(ContentHubEntityEligibilityEvent $event): void {
    if (!$this->personalizedContentPushIsActive()) {
      return;
    }

    $entity = $event->getEntity();
    // If the entity view configuration on Publisher settings page is set for
    // the entity in question, the entity is qualified to be processed.
    if (empty($this->getEntityViewModesSettingValue($entity))) {
      $event->setEligibility(FALSE);
      $event->stopPropagation();
      return;
    }

    $entities = [];
    $cdfs = $this->commonActions->getEntityCdf($entity, $entities, TRUE, FALSE);
    foreach ($cdfs as $cdf) {
      if ($cdf->getType() !== 'rendered_entity') {
        continue;
      }

      $source_uuid = $this->getCdfEntityAttributeValue($cdf, 'source_entity');
      if ($source_uuid === $entity->uuid()) {
        $event->setEligibility(TRUE);
        $event->setCalculateDependencies(FALSE);
        return;
      }
    }

    $event->setEligibility(FALSE);
    $event->stopPropagation();
  }

  /**
   * Removes cdfs that are not part of the personalized content scheme.
   *
   * @param \Drupal\acquia_contenthub\Event\PrunePublishCdfEntitiesEvent $event
   *   The current publishing event.
   */
  public function onPrunePublishCdfEntities(PrunePublishCdfEntitiesEvent $event): void {
    if (!$this->personalizedContentPushIsActive()) {
      return;
    }

    $cdf_document = $event->getDocument();
    $cdfs_to_remove = $cdf_document->getEntities();
    $rendered_entity = NULL;
    foreach ($cdfs_to_remove as $uuid => $cdf_entity) {
      if ($cdf_entity->getType() === 'rendered_entity') {
        $rendered_entity = $cdf_entity;
        unset($cdfs_to_remove[$uuid]);
      }
    }

    // Extra sanity check. The personalized content push was set to true,
    // therefore, if for some reason the rendered entity is missing, stop the
    // execution.
    if (!$rendered_entity) {
      return;
    }

    $se_uuid = $this->getCdfEntityAttributeValue($rendered_entity, 'source_entity');
    if (!$se_uuid) {
      return;
    }

    $cdfs_to_keep = [$se_uuid];
    $source_entity = $cdfs_to_remove[$se_uuid];
    $tags = $this->getCdfEntityAttributeValue($source_entity, 'tags');
    if (is_array($tags)) {
      array_push($cdfs_to_keep, ...$tags);
    }

    $cdfs_to_remove = array_diff(array_keys($cdfs_to_remove), $cdfs_to_keep);
    foreach ($cdfs_to_remove as $uuid) {
      $cdf_document->removeCdfEntity($uuid);
    }
  }

  /**
   * Returns an arbitrary attribute's value.
   *
   * @param \Acquia\ContentHubClient\CDF\CDFObject $cdf
   *   The cdf object in hand.
   * @param string $attribute
   *   The attribute to get from the cdf.
   *
   * @return mixed|null
   *   The attribute value could be several type. If the there's no value, or
   *   the attribute doesn't exist for some reason, return NULL.
   */
  private function getCdfEntityAttributeValue(CDFObject $cdf, string $attribute) {
    $attribute_obj = $cdf->getAttribute($attribute);
    if (!$attribute_obj) {
      return NULL;
    }

    $attr_val = $attribute_obj->getValue();
    return $attr_val[CDFObject::LANGUAGE_UNDETERMINED] ?? NULL;
  }

  /**
   * Determines if personalized content push rule is active.
   *
   * @return bool
   *   TRUE if active.
   */
  private function personalizedContentPushIsActive(): bool {
    return (bool) $this->publisherSettings->get(ContentPublishingForm::$pushSettingField);
  }

}
