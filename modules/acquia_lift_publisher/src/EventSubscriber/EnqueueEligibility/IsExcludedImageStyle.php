<?php

namespace Drupal\acquia_lift_publisher\EventSubscriber\EnqueueEligibility;

use Drupal\acquia_contenthub_publisher\ContentHubPublisherEvents;
use Drupal\acquia_contenthub_publisher\Event\ContentHubEntityEligibilityEvent;
use Drupal\Core\Entity\EntityInterface;
use Drupal\image\Entity\ImageStyle;
use Symfony\Component\EventDispatcher\EventSubscriberInterface;

/**
 * Class IsExcludedImageStyle.
 *
 * Prevents to enqueue some image styles.
 *
 * @package Drupal\acquia_lift_publisher\EventSubscriber\EnqueueEligibility
 */
class IsExcludedImageStyle implements EventSubscriberInterface {

  /**
   * {@inheritdoc}
   */
  public static function getSubscribedEvents() {
    $events[ContentHubPublisherEvents::ENQUEUE_CANDIDATE_ENTITY][] = [
      'onEnqueueCandidateEntity',
      50,
    ];

    return $events;
  }

  /**
   * Reacts on ContentHubPublisherEvents::ENQUEUE_CANDIDATE_ENTITY event.
   *
   * @param \Drupal\acquia_contenthub_publisher\Event\ContentHubEntityEligibilityEvent $event
   *   Event.
   *
   * @throws \Exception
   */
  public function onEnqueueCandidateEntity(ContentHubEntityEligibilityEvent $event) {
    $entity = $event->getEntity();
    if (FALSE == $this->isExcluded($entity)) {
      return;
    }

    $event->setEligibility(FALSE);
    $event->stopPropagation();
  }

  /**
   * Checks if current entity must be excluded from export queue.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   Entity.
   *
   * @return bool
   *   TRUE if entity is image style from acquia_lift_publisher module.
   */
  protected function isExcluded(EntityInterface $entity) {
    if (FALSE === ($entity instanceof ImageStyle)) {
      return FALSE;
    }

    $excluded_items = ['acquia_lift_publisher_preview_image'];
    return in_array($entity->getOriginalId(), $excluded_items);
  }

}
