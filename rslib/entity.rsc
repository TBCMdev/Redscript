use math;
use uuid;

object entity
{
required
    air: int;
    CustomName: text_component;
    CustomNameVisible: bool;
    data: object;
    fall_distance: double; // ?
    Fire: short;
    Glowing: bool;
    HasVisualFire: bool;
    id: string?; // does not exist for player
    Invulnerable: bool;
    Motion: dvector3;
    NoGravity: bool;
    OnGround: bool;
    Passengers: entity[];
    PortalCooldown: int;
    Pos: dvector3;
    Rotation: fvector2; // yaw, pitch
    Silent: bool;
    Tags: string[];
    TicksFrozen: int?; // is optional
    UUID: uuid;
}