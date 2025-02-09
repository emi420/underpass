CREATE INDEX nodes_version_idx ON public.nodes (version);
CREATE INDEX ways_poly_version_idx ON public.ways_poly (version);
CREATE INDEX ways_line_version_idx ON public.ways_line (version);
CREATE INDEX idx_changesets_hashtags ON public.changesets USING gin(hashtags);

