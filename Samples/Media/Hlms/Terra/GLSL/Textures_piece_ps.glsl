
@undefpiece( kS )
@piece( kS )vec3( 1, 1, 1 )@end

@foreach( detail_maps_normal, n )
	@undefpiece( SampleDetailMapNm@n )
	@piece( SampleDetailMapNm@n )getTSDetailNormal( textureMaps[@value(detail_map_nm@n_idx)], vec3( inPs.uv0.xy * material.detailOffsetScale[@n].zw +
																									material.detailOffsetScale[@n].xy , @value(detail_map_nm@n_slice_idx) ) )@end
@end