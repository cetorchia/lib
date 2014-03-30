#
# ASCII database parsing
#
# (c) 2010 Carlos E. Torchia
#

package DB;
use strict;

#
# Get the rows of the table where the given fields are equal to the query.
#

sub get
{
	my $filename = shift;
	my $queryRef = shift;
	my %query = %{$queryRef};

	my @rows = ();

	open(fileHandle, "<$filename");

	#
	# Get the field indices
	#

	my $line = <fileHandle>;
	my $fields = getFields($line);
	my $fieldIndices = getFieldIndices($fields);

	#
	# Get those rows with the given value
	#

	while(($line = <fileHandle>))
	{
		$fields = getFields($line);
		my $row = getRow($fields, $fieldIndices);

		my $matches = 1;
		foreach my $name (keys %query)
		{
			if(!matches($row->{$name}, $query{$name}))
			{
				$matches = 0;
				last;
			}
		}

		if($matches)
		{
			push @rows, $row;
		}
	}

	close(fileHandle);

	return \@rows;
}

#
# Given the field indices and fields, maps each field name to its value.
#

sub getRow
{
	my $fieldsRef = shift;
	my @fields = @{$fieldsRef};
	my $fieldIndicesRef = shift;
	my %fieldIndices = %{$fieldIndicesRef};
	my %row = ();

	foreach my $name (keys %fieldIndices)
	{
		my $index = $fieldIndices{$name};
		my $value = $fields[$index];
		$row{$name} = $value;
	}

	return \%row;
}

#
# Returns a map of each field name to its index
#

sub getFieldIndices
{
	my $fieldsRef = shift;
	my @fields = @{$fieldsRef};

	my %indices = ();
	my $i = 0;

	foreach my $field (@fields)
	{
		if($field =~ /^\"(.*)\"$/)
		{
			$indices{$1} = $i;
		}

		$i = $i + 1;
	}

	return \%indices;
}

#
# Returns whether the field matches the value
#

sub matches
{
	my $x = shift;
	my $value = shift;


	if($x eq $value)
	{
		return 1;
	}

	elsif($x =~ /^\".*\"$/)
	{
		my $xl = lc($x);
		my $vl = lc($value);
		if(index($xl,$vl) >= 0)
		{
			return 1;
		}
	}

	return 0;
}

#
# Converts a $-separated string into an array with the delimited values.
#

sub getFields
{
	my $line = shift;

	my @row = split(/\$/, $line);
	return \@row;
}

1;
