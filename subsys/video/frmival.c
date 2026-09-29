/*
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(video_frmival, CONFIG_VIDEO_LOG_LEVEL);

int video_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	if (dev == NULL || frmival == NULL) {
		return -EINVAL;
	}

	return video_driver_set_frmival(dev, frmival);
}

int video_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	if (dev == NULL || frmival == NULL) {
		return -EINVAL;
	}

	return video_driver_get_frmival(dev, frmival);
}

int video_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	if (dev == NULL || fie == NULL) {
		return -EINVAL;
	}

	return video_driver_enum_frmival(dev, fie);
}

int video_closest_frmival_stepwise(const struct video_frmival_stepwise *stepwise,
				   uint64_t desired,
				   uint64_t *match)
{
	uint64_t goal;

	if (stepwise == NULL || match == NULL) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(stepwise->step != 0U);
	/* Prevent division by zero */
	if (stepwise->step == 0U) {
		return -ERANGE;
	}

	/* Saturate the desired value to the min/max supported */
	goal = CLAMP(desired, stepwise->min, stepwise->max);

	/* Compute the closest match */
	*match = stepwise->min +
		 DIV_ROUND_CLOSEST(goal - stepwise->min, stepwise->step) * stepwise->step;

	return 0;
}

int video_closest_frmival(const struct device *dev, struct video_frmival_enum *match)
{
	if (dev == NULL || match == NULL || match->type == VIDEO_FRMIVAL_TYPE_STEPWISE) {
		return -EINVAL;
	}

	struct video_frmival desired = match->discrete;
	struct video_frmival_enum fie = {.format = match->format};
	uint64_t best_diff_nsec = UINT64_MAX;
	uint64_t goal_nsec = video_frmival_nsec(&desired);

	for (fie.index = 0; video_enum_frmival(dev, &fie) == 0; fie.index++) {
		struct video_frmival tmp = {0};
		uint64_t diff_nsec = 0;
		uint64_t tmp_nsec = 0;
		int ret;

		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			tmp = fie.discrete;
			tmp_nsec = video_frmival_nsec(&tmp);
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			ret = video_closest_frmival_stepwise(&fie.stepwise, goal_nsec, &tmp_nsec);
			if (ret != 0) {
				continue;
			}
			tmp.numerator = tmp_nsec;
			tmp.denominator = NSEC_PER_SEC;
			break;
		default:
			CODE_UNREACHABLE;
		}

		diff_nsec = tmp_nsec > goal_nsec ? tmp_nsec - goal_nsec : goal_nsec - tmp_nsec;

		if (diff_nsec < best_diff_nsec) {
			best_diff_nsec = diff_nsec;
			match->index = fie.index;
			match->discrete = tmp;
		}

		if (diff_nsec == 0) {
			/* Exact match, stop searching a better match */
			break;
		}
	}

	return 0;
}
