<?php
/**
 * @file
 * Includes all necessary classes for Acquia Lift.
 * Also defines special Exception types.
 */

class AcquiaLiftException extends Exception {}
class AcquiaLiftServerErrorException extends AcquiaLiftException {}
class AcquiaLiftClientErrorException extends AcquiaLiftException {}
class AcquiaLiftNotFoundException extends AcquiaLiftClientErrorException {}
class AcquiaLiftForbiddenException extends AcquiaLiftClientErrorException {}
class AcquiaLiftCredsException extends AcquiaLiftException {}
