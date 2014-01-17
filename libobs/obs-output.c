/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "obs.h"
#include "obs-data.h"

bool load_output_info(void *module, const char *module_name,
		const char *id, struct output_info *info)
{
	LOAD_MODULE_SUBFUNC(getname, true);
	LOAD_MODULE_SUBFUNC(create, true);
	LOAD_MODULE_SUBFUNC(destroy, true);
	LOAD_MODULE_SUBFUNC(update, true);
	LOAD_MODULE_SUBFUNC(start, true);
	LOAD_MODULE_SUBFUNC(stop, true);
	LOAD_MODULE_SUBFUNC(active, true);

	LOAD_MODULE_SUBFUNC(pause, false);

	info->id = id;
	return true;
}

static inline const struct output_info *find_output(const char *id)
{
	size_t i;
	for (i = 0; i < obs->output_types.num; i++)
		if (strcmp(obs->output_types.array[i].id, id) == 0)
			return obs->output_types.array+i;

	return NULL;
}

obs_output_t obs_output_create(const char *id, const char *name,
		const char *settings)
{
	const struct output_info *info = find_output(id);
	struct obs_output *output;

	if (!info) {
		blog(LOG_WARNING, "Output '%s' not found", id);
		return NULL;
	}

	output = bmalloc(sizeof(struct obs_output));
	output->callbacks = *info;
	output->data = info->create(settings, output);
	if (!output->data) {
		bfree(output);
		return NULL;
	}

	output->name = bstrdup(name);
	dstr_init_copy(&output->settings, settings);

	pthread_mutex_lock(&obs->data.outputs_mutex);
	da_push_back(obs->data.outputs, &output);
	pthread_mutex_unlock(&obs->data.outputs_mutex);
	return output;
}

void obs_output_destroy(obs_output_t output)
{
	if (output) {
		pthread_mutex_lock(&obs->data.outputs_mutex);
		da_erase_item(obs->data.outputs, &output);
		pthread_mutex_unlock(&obs->data.outputs_mutex);

		output->callbacks.destroy(output->data);
		dstr_free(&output->settings);
		bfree(output->name);
		bfree(output);
	}
}

bool obs_output_start(obs_output_t output)
{
	return output->callbacks.start(output->data);
}

void obs_output_stop(obs_output_t output)
{
	output->callbacks.stop(output->data);
}

bool obs_output_active(obs_output_t output)
{
	return output->callbacks.active(output);
}

void obs_output_update(obs_output_t output, const char *settings)
{
	if (output->callbacks.update)
		output->callbacks.update(output->data, settings);
}

bool obs_output_canpause(obs_output_t output)
{
	return output->callbacks.pause != NULL;
}

void obs_output_pause(obs_output_t output)
{
	if (output->callbacks.pause)
		output->callbacks.pause(output->data);
}

void obs_output_save_settings(obs_output_t output, const char *settings)
{
	dstr_copy(&output->settings, settings);
}
