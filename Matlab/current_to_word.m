function result=current_to_word(value)
    value=value*1000; % Value in A
    result=fix((value/2.5)*2^16);
end