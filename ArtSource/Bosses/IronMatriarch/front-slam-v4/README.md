# Front-facing slam: detail and fringe correction

Replaces the rejected soft v3 front slam only. Original side views, other attacks,
combat numbers and event timing are unchanged.

- Built-in image generation: four smaller pose groups, original front pose
  reference plus original left-view style reference. Exact prompts: prompts.json.
- Source images are 1254 x 1254 each, with substantially larger individual poses
  than the sixteen-pose v3 source. They are not represented as 2K source images.
- User explicitly approved local removal of translucent fringe. Source alpha
  below 160 is removed; the opaque interior RGB is not recolored. After atlas
  resampling, alpha is made binary at 128. Export asserts zero translucent pixels.
- Final asset uses nearest filtering, no mipmaps, lossless UI compression and
  never-stream residency. Other texture filtering remains unchanged.
- Measured crop windows and explicit pose selection are in
  Tools/register_iron_front_v4.py. The original registered target bounds are
  retained to avoid size drift on repeated exports.
- Flying poses are selected by wing phase rather than generated reading order.
- Reproduce: Tools/register_iron_front_v4.py, then UE Python commandlet with
  Tools/import_iron_front_only.py. Full prepare/import also preserves this fix.

Row 2 was re-framed with the built-in tool to restore its clipped wing tip:

Precise framing repair only. These four mechanical five-headed dragon poses must remain identical in detail, proportions, lighting, pose, shading and pixel style. Put them on a larger transparent canvas with 60 pixels clear transparent padding around EVERY sprite in this 2x2 sheet. Restore the clipped far left wing tip of bottom-left sprite; complete wing silhouette. All wings tails and feet fully inside image edges. No new glow, no blur, no smooth repainting. Keep all four poses unchanged. Top-left wings raised and feet curled, top-right flight spread wings and feet curled, bottom-left flight spread wings feet down, bottom-right crouch. No text/background.

Selected generation identifiers: row 1 bd9927e9-5d9a-4fa5-8aad-db2013bca2e5;
row 2 20334283-f1e6-4358-a03b-e2f9c13cbe70; row 3
74107c7d-2642-4860-9101-fc9712a9e683; row 4
8b3a9971-8e06-45ad-9669-649e01ccf224.

All selected images are saved alongside this document; generated cache files
are not runtime dependencies. The consumed atlas is
../runtime-v1/Iron_flying_slam_front.png and the UE asset is
/Game/Art/IronMatriarch/Iron_flying_slam_front.
