/*
    This file is part of darktable,
    Copyright (C) 2024 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "common/image.h"
#include "common/image_cache.h"
#include "control/control.h"
#include <glib.h>

gboolean dt_export_gimp_file(const dt_imgid_t imgid)
{
  const gboolean thumb = dt_check_gimpmode("thumb");
  char *tmp_directory = g_dir_make_tmp("darktable_XXXXXX", NULL);
  char path[PATH_MAX]= { 0 };

  snprintf(path, sizeof(path), "%s/%s", tmp_directory, thumb ? "thumb" : "image");
  g_free(tmp_directory);

  // init the export data structures
  dt_imageio_module_storage_t *storage = dt_imageio_get_storage_by_name("disk");
  if(storage == NULL) return FALSE;

  dt_imageio_module_data_t *sdata = storage->get_params(storage);
  if(sdata == NULL) return FALSE;

  // and now for the really ugly hacks. don't tell your children about this one or they won't sleep at night
  // any longer ...
  g_strlcpy((char *)sdata, path, DT_MAX_PATH_FOR_PARAMS);

  dt_imageio_module_format_t *format = dt_imageio_get_format_by_name(thumb ? "jpeg" : "exr");
  if(format == NULL) return FALSE;

  dt_imageio_module_data_t *fdata = format->get_params(format);
  if(fdata == NULL) return FALSE;

  // For disk exporting and used formats we dont have to check dimensions
  const size_t dim = MIN(MAX(darktable.gimp.size, 32), 1024);

  fdata->max_width = thumb ? dim : 0;
  fdata->max_height = thumb ? dim : 0;
  fdata->style[0] = '\0';
  fdata->style_append = FALSE;

  storage->store(storage, sdata, imgid, format, fdata, 1, 1,
                  thumb ? FALSE : TRUE, // high_quality,
                  FALSE, // never upscale
                  thumb ? FALSE : TRUE, // export_masks
                  thumb ? DT_COLORSPACE_SRGB : DT_COLORSPACE_LIN_REC709,
                  NULL,  // icc_filename
                  DT_INTENT_PERCEPTUAL,
                  NULL);  // &metadata
  fprintf(stdout, "<<<gimp\n%s%s\n", path, thumb ? ".jpg" : ".exr");
  if(thumb)
  {
    dt_image_t *image = dt_image_cache_get(darktable.image_cache, imgid, 'r');
    fprintf(stdout, "%i %i\n", image->width, image->height);
    dt_image_cache_read_release(darktable.image_cache, image);
  }
  fprintf(stdout, "gimp>>>\n");
  return TRUE;
}

dt_imgid_t dt_gimp_load_image(const char *file)
{
  gboolean single;
  darktable.gimp.imgid = dt_load_from_string(file, FALSE, &single);
  darktable.gimp.error = !single;
  return darktable.gimp.imgid;
}

dt_imgid_t dt_gimp_load_darkroom(const char *file)
{
  gboolean single;
  darktable.gimp.imgid = dt_load_from_string(file, TRUE, &single);
  darktable.gimp.error = !single;
  return darktable.gimp.imgid;
}
